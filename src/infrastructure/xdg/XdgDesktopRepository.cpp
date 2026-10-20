#include "infrastructure/xdg/XdgDesktopRepository.hpp"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

namespace cachy_dock::infrastructure::xdg {

XdgDesktopRepository::XdgDesktopRepository() = default;
XdgDesktopRepository::~XdgDesktopRepository() = default;

std::optional<domain::entities::DockItem> XdgDesktopRepository::resolveDesktopFile(const std::string& desktopPathOrName) const {
    QString resolvedPath = QString::fromStdString(desktopPathOrName);

    if (!QFileInfo::exists(resolvedPath)) {
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

bool XdgDesktopRepository::launch(const std::string& execCommand, bool useDiscreteGpu) {
    if (execCommand.empty()) {
        return false;
    }

    QString cmd = QString::fromStdString(execCommand);
    // Strip XDG field codes (%f, %F, %u, %U, etc.)
    cmd.remove(QRegularExpression(QStringLiteral("%[a-zA-Z]")));
    cmd = cmd.trimmed();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (useDiscreteGpu) {
        // Environment variables for CachyOS hybrid GPU offloading
        env.insert("DRI_PRIME", "1");
        env.insert("__NV_PRIME_RENDER_OFFLOAD", "1");
        env.insert("__GLX_VENDOR_LIBRARY_NAME", "nvidia");
        env.insert("__VK_LAYER_NV_optimus", "NVIDIA_only");
    }

    QStringList args = cmd.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (args.isEmpty()) {
        return false;
    }

    const QString program = args.takeFirst();
    return QProcess::startDetached(program, args, QString(), nullptr);
}

bool XdgDesktopRepository::launchDesktopId(const std::string& desktopId, bool useDiscreteGpu) {
    QString path = QString::fromStdString(desktopId);
    if (!QFileInfo::exists(path)) {
        return false;
    }

    QSettings desktopFile(path, QSettings::IniFormat);
    desktopFile.beginGroup("Desktop Entry");
    QString exec = desktopFile.value("Exec").toString();
    return launch(exec.toStdString(), useDiscreteGpu);
}

} // namespace cachy_dock::infrastructure::xdg
