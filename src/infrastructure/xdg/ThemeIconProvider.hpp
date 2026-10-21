#pragma once

#include <QQuickImageProvider>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QThread>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>
#include <mutex>
#include <unordered_map>

namespace cachy_dock::infrastructure::xdg {

class ThemeIconProvider : public QQuickImageProvider {
public:
    ThemeIconProvider()
        : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override {
        const int width = requestedSize.width() > 0 ? requestedSize.width() : 128;
        const int height = requestedSize.height() > 0 ? requestedSize.height() : 128;

        if (size) {
            *size = QSize(width, height);
        }

        // Check in-memory cache
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_cache.find(id);
            if (it != m_cache.end()) {
                if (it->second.width() == width && it->second.height() == height) {
                    return it->second;
                }
                return it->second.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }

        QImage resultImage;

        // QIcon::fromTheme touches QGuiApplication's internal icon theme engine and installs event filters.
        // It MUST be invoked on the GUI thread to prevent cross-thread parent/eventFilter errors.
        if (qApp && QThread::currentThread() != qApp->thread()) {
            QMetaObject::invokeMethod(qApp, [this, id, width, height, &resultImage]() {
                resultImage = loadIcon(id, width, height);
            }, Qt::BlockingQueuedConnection);
        } else {
            resultImage = loadIcon(id, width, height);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache[id] = resultImage;
        }

        return resultImage;
    }

private:
    QImage loadIcon(const QString &id, int width, int height) {
        QString cleanId = id;
        if (cleanId.endsWith(QLatin1String(".desktop"), Qt::CaseInsensitive)) {
            cleanId.chop(8);
        }

        QIcon icon = QIcon::fromTheme(cleanId);
        if (icon.isNull() && cleanId != id) {
            icon = QIcon::fromTheme(id);
        }
        if (icon.isNull()) {
            icon = QIcon(id);
        }

        // Try standard KDE reverse-DNS prefix (e.g. "dolphin" -> "org.kde.dolphin")
        if (icon.isNull() && !cleanId.contains(QLatin1Char('.'))) {
            icon = QIcon::fromTheme(QLatin1String("org.kde.") + cleanId);
        }

        // Specific fallbacks for common apps (Dolphin, Konsole)
        if (icon.isNull() && (cleanId == QLatin1String("dolphin") || cleanId == QLatin1String("org.kde.dolphin"))) {
            icon = QIcon::fromTheme(QLatin1String("system-file-manager"));
            if (icon.isNull() && QFileInfo::exists(QStringLiteral("/usr/share/icons/hicolor/scalable/apps/org.kde.dolphin.svg"))) {
                icon = QIcon(QStringLiteral("/usr/share/icons/hicolor/scalable/apps/org.kde.dolphin.svg"));
            }
        } else if (icon.isNull() && (cleanId == QLatin1String("konsole") || cleanId == QLatin1String("org.kde.konsole"))) {
            icon = QIcon::fromTheme(QLatin1String("utilities-terminal"));
        }

        // If cleanId is a reverse-DNS appId (e.g. com.microsoft.VSCode), resolve via its .desktop file
        if (icon.isNull()) {
            static const auto appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
            for (const auto &dir : appDirs) {
                QString candidate = dir + QLatin1Char('/') + cleanId;
                if (!candidate.endsWith(QLatin1String(".desktop"), Qt::CaseInsensitive)) {
                    candidate += QLatin1String(".desktop");
                }
                if (QFileInfo::exists(candidate)) {
                    QSettings desktopFile(candidate, QSettings::IniFormat);
                    desktopFile.beginGroup("Desktop Entry");
                    const QString desktopIcon = desktopFile.value("Icon").toString();
                    if (!desktopIcon.isEmpty() && desktopIcon != cleanId && desktopIcon != id) {
                        return loadIcon(desktopIcon, width, height);
                    }
                    break;
                }
            }
        }

        if (icon.isNull()) {
            static const QStringList fallbackDirs = {
                QStringLiteral("/usr/share/pixmaps"),
                QStringLiteral("/usr/share/icons/hicolor/scalable/apps"),
                QStringLiteral("/usr/share/icons/hicolor/48x48/apps"),
                QStringLiteral("/usr/share/icons/hicolor/128x128/apps"),
                QStringLiteral("/usr/share/icons/hicolor/256x256/apps"),
                QStringLiteral("/usr/share/icons/hicolor/512x512/apps"),
                QStringLiteral("/usr/share/icons/breeze-dark/apps/48"),
                QStringLiteral("/usr/share/icons/breeze-dark/places/48"),
                QStringLiteral("/usr/share/icons/breeze/apps/48"),
                QStringLiteral("/usr/share/icons/breeze/places/48")
            };
            static const QStringList extensions = {
                QStringLiteral(".png"),
                QStringLiteral(".svg"),
                QStringLiteral(".xpm")
            };

            // Try exact cleanId, lowercase, and reverse-DNS suffix (e.g. com.microsoft.VSCode -> vscode)
            QStringList namesToTry = { cleanId, cleanId.toLower() };
            if (cleanId != id) {
                namesToTry << id << id.toLower();
            }
            if (cleanId == QLatin1String("dolphin")) {
                namesToTry << QStringLiteral("org.kde.dolphin");
            } else if (cleanId == QLatin1String("org.kde.dolphin")) {
                namesToTry << QStringLiteral("dolphin");
            }
            const QString suffix = cleanId.section(QLatin1Char('.'), -1);
            if (!suffix.isEmpty() && suffix != cleanId) {
                namesToTry << suffix << suffix.toLower();
            }

            for (const auto &name : namesToTry) {
                for (const auto &dir : fallbackDirs) {
                    for (const auto &ext : extensions) {
                        const QString candidate = dir + QLatin1Char('/') + name + ext;
                        if (QFileInfo::exists(candidate)) {
                            icon = QIcon(candidate);
                            if (!icon.isNull()) break;
                        }
                    }
                    if (!icon.isNull()) break;
                }
                if (!icon.isNull()) break;
            }
        }

        if (!icon.isNull()) {
            QPixmap px = icon.pixmap(width, height);
            if (!px.isNull()) {
                return px.toImage();
            }
        }

        // Generate a high-DPI glassmorphic fallback badge
        QImage fallback(width, height, QImage::Format_ARGB32_Premultiplied);
        fallback.fill(Qt::transparent);

        QPainter painter(&fallback);
        painter.setRenderHint(QPainter::Antialiasing);

        const uint hash = qHash(id);
        const int hue = hash % 360;
        const QColor bgColor = QColor::fromHsv(hue, 160, 220);

        painter.setBrush(bgColor);
        painter.setPen(QPen(QColor(255, 255, 255, 120), 2));
        painter.drawRoundedRect(4, 4, width - 8, height - 8, 22, 22);

        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPixelSize(height / 2);
        font.setBold(true);
        painter.setFont(font);

        QString letter = id.section(QLatin1Char('.'), -1);
        if (letter.isEmpty()) {
            letter = id;
        }
        const QString firstChar = letter.left(1).toUpper();
        painter.drawText(fallback.rect(), Qt::AlignCenter, firstChar.isEmpty() ? QStringLiteral("?") : firstChar);

        return fallback;
    }

    std::mutex m_mutex;
    std::unordered_map<QString, QImage> m_cache;
};

} // namespace cachy_dock::infrastructure::xdg
