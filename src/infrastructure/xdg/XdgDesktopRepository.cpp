#include "infrastructure/xdg/XdgDesktopRepository.hpp"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

namespace cachy_dock::infrastructure::xdg {

XdgDesktopRepository::XdgDesktopRepository() = default;
XdgDesktopRepository::~XdgDesktopRepository() = default;

std::optional<domain::entities::DockItem> XdgDesktopRepository::resolveDesktopFile(const std::string& desktopPathOrName) {
    QString resolvedPath = QString::fromStdString(desktopPathOrName);

    if (resolvedPath.startsWith(QLatin1Char('/'))) {
        if (!QFileInfo::exists(resolvedPath)) {
            return std::nullopt;
        }
    } else if (!QFileInfo::exists(resolvedPath)) {
        // Search standard applications directories
        const auto appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
        bool found = false;
        for (const auto &dir : appDirs) {
            QString candidate = dir + "/" + resolvedPath;
            if (!candidate.endsWith(".desktop")) {
                candidate += ".desktop";
            }
            if (QFileInfo::exists(candidate)) {
                resolvedPath = candidate;
                found = true;
                break;
            }
        }
        if (!found) {
            // Try reverse-DNS matching (e.g. "dolphin.desktop" or "dolphin" -> "org.kde.dolphin.desktop")
            const QString base = QFileInfo(resolvedPath).baseName();
            if (!base.isEmpty()) {
                for (const auto &dir : appDirs) {
                    QDir d(dir);
                    const auto matches = d.entryList({QLatin1String("*.") + base + QLatin1String(".desktop")}, QDir::Files);
                    if (!matches.isEmpty()) {
                        resolvedPath = d.filePath(matches.first());
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            return std::nullopt;
        }
    }

    QSettings desktopFile(resolvedPath, QSettings::IniFormat);
    desktopFile.beginGroup("Desktop Entry");

    if (desktopFile.value("NoDisplay", false).toBool()) {
        return std::nullopt;
    }

    QString name = desktopFile.value("Name").toString();
    QString icon = desktopFile.value("Icon").toString();

    if (name.isEmpty()) {
        name = QFileInfo(resolvedPath).baseName();
    }

    domain::entities::DockItem item{
        .id = resolvedPath.toStdString(),
        .name = name.toStdString(),
        .iconName = icon.toStdString(),
        .type = domain::entities::DockItemType::AppLauncher,
        .isPinned = true,
        .isRunning = false,
        .isFocused = false,
        .runningWindowCount = 0
    };

    return item;
}

std::vector<domain::entities::DockItem> XdgDesktopRepository::getDefaultPinnedApps() const {
    std::vector<domain::entities::DockItem> items;

    // Typical default launchers on CachyOS / KDE Plasma
    const std::vector<std::string> candidates = {
        "org.kde.dolphin.desktop",
        "cachy-browser.desktop",
        "google-chrome.desktop",
        "firefox.desktop",
        "org.kde.konsole.desktop",
        "kate.desktop",
        "systemsettings.desktop"
    };

    for (const auto &candidate : candidates) {
        auto item = resolveDesktopFile(candidate);
        if (item.has_value()) {
            items.push_back(std::move(*item));
        }
    }

    // Add separator
    domain::entities::DockItem sep{
        .id = "separator_recent",
        .name = "Separator",
        .iconName = "",
        .type = domain::entities::DockItemType::Separator,
        .isPinned = true,
        .isRunning = false,
        .isFocused = false
    };
    items.push_back(sep);

    // Add Document Stack for Downloads
    const QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    domain::entities::DockItem downloadsStack{
        .id = downloadsPath.toStdString(),
        .name = "Downloads",
        .iconName = "folder-download",
        .type = domain::entities::DockItemType::DocumentStack,
        .isPinned = true,
        .isRunning = false,
        .isFocused = false
    };
    items.push_back(downloadsStack);

    return items;
}

bool XdgDesktopRepository::isSystemdUserSessionAvailable() {
    if (QStandardPaths::findExecutable(QStringLiteral("systemd-run")).isEmpty()) {
        return false;
    }
    const QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtimeDir.isEmpty()) {
        return false;
    }
    return QFileInfo::exists(runtimeDir + QStringLiteral("/systemd/private"));
}

LaunchCommandSpec XdgDesktopRepository::buildLaunchCommand(
    const std::string& execCommand,
    bool useDiscreteGpu,
    bool isolateInSystemdScope)
{
    LaunchCommandSpec spec;
    if (execCommand.empty()) {
        return spec;
    }

    QString cmd = QString::fromStdString(execCommand);
    // Strip XDG field codes (%f, %F, %u, %U, %i, %c, %k, etc.)
    cmd.remove(QRegularExpression(QStringLiteral("%[a-zA-Z]")));
    cmd = cmd.trimmed();

    QStringList args = QProcess::splitCommand(cmd);
    if (args.isEmpty()) {
        return spec;
    }

    const QString targetProgram = args.takeFirst();

    if (useDiscreteGpu) {
        spec.envOverrides = {
            {"DRI_PRIME", "1"},
            {"__NV_PRIME_RENDER_OFFLOAD", "1"},
            {"__GLX_VENDOR_LIBRARY_NAME", "nvidia"},
            {"__VK_LAYER_NV_optimus", "NVIDIA_only"}
        };
    }

    const QString systemdRunPath = QStandardPaths::findExecutable(QStringLiteral("systemd-run"));
    if (isolateInSystemdScope && !systemdRunPath.isEmpty()) {
        spec.program = systemdRunPath.toStdString();
        spec.usesSystemdScope = true;
        spec.arguments = {
            "--user",
            "--scope",
            "--collect",
            "--quiet",
            "--slice=app.slice"
        };
        for (const auto& [key, val] : spec.envOverrides) {
            spec.arguments.push_back("--setenv=" + key + "=" + val);
        }
        spec.arguments.push_back("--");
        spec.arguments.push_back(targetProgram.toStdString());
        for (const auto& arg : args) {
            spec.arguments.push_back(arg.toStdString());
        }
    } else {
        spec.program = targetProgram.toStdString();
        spec.usesSystemdScope = false;
        for (const auto& arg : args) {
            spec.arguments.push_back(arg.toStdString());
        }
    }

    return spec;
}

bool XdgDesktopRepository::launch(const std::string& execCommand, bool useDiscreteGpu) {
    if (execCommand.empty()) {
        return false;
    }

    QString cmd = QString::fromStdString(execCommand);
    cmd.remove(QRegularExpression(QStringLiteral("%[a-zA-Z]")));
    cmd = cmd.trimmed();

    const QStringList parsedArgs = QProcess::splitCommand(cmd);
    if (parsedArgs.isEmpty()) {
        return false;
    }

    // Verify target binary exists before wrapping in systemd-run so invalid commands fail immediately
    const QString rawProgram = parsedArgs.first();
    const QString resolvedBinary = rawProgram.startsWith(QLatin1Char('/'))
        ? (QFileInfo(rawProgram).isExecutable() ? rawProgram : QString())
        : QStandardPaths::findExecutable(rawProgram);
    if (resolvedBinary.isEmpty()) {
        return false;
    }

    const LaunchCommandSpec spec = buildLaunchCommand(
        execCommand,
        useDiscreteGpu,
        isSystemdUserSessionAvailable());
    if (spec.program.empty()) {
        return false;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const auto& [key, val] : spec.envOverrides) {
        env.insert(QString::fromStdString(key), QString::fromStdString(val));
    }

    QStringList qArgs;
    qArgs.reserve(static_cast<qsizetype>(spec.arguments.size()));
    for (const auto& arg : spec.arguments) {
        qArgs.append(QString::fromStdString(arg));
    }

    QProcess process;
    process.setProgram(QString::fromStdString(spec.program));
    process.setArguments(qArgs);
    process.setProcessEnvironment(env);
    process.setWorkingDirectory(QDir::homePath());
    return process.startDetached();
}

bool XdgDesktopRepository::launchDesktopId(const std::string& desktopId, bool useDiscreteGpu) {
    QString path = QString::fromStdString(desktopId);
    if (path.startsWith(QLatin1String("~/"))) {
        path = QDir::homePath() + path.mid(1);
    }
    if (path.compare(QLatin1String("downloads"), Qt::CaseInsensitive) == 0) {
        path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    } else if (path.compare(QLatin1String("documents"), Qt::CaseInsensitive) == 0) {
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QFileInfo info(path);
    if (info.exists() && info.isDir()) {
        if (!QStandardPaths::findExecutable(QStringLiteral("xdg-open")).isEmpty()) {
            QString escapedPath = path;
            escapedPath.replace(QLatin1Char('"'), QStringLiteral("\\\""));
            return launch(QStringLiteral("xdg-open \"%1\"").arg(escapedPath).toStdString(), useDiscreteGpu);
        }
        return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }

    QString resolvedPath = path;
    if (!info.exists()) {
        auto item = resolveDesktopFile(desktopId);
        if (item.has_value()) {
            resolvedPath = QString::fromStdString(item->id);
        } else {
            return false;
        }
    }

    QSettings desktopFile(resolvedPath, QSettings::IniFormat);
    desktopFile.beginGroup("Desktop Entry");
    QString exec = desktopFile.value("Exec").toString();
    return launch(exec.toStdString(), useDiscreteGpu);
}

} // namespace cachy_dock::infrastructure::xdg
