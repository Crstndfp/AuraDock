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
        if (QThread::currentThread() != qApp->thread()) {
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
        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull()) {
            icon = QIcon(id);
        }

        if (icon.isNull()) {
            static const QStringList fallbackDirs = {
                QStringLiteral("/usr/share/pixmaps"),
                QStringLiteral("/usr/share/icons/hicolor/scalable/apps"),
                QStringLiteral("/usr/share/icons/hicolor/48x48/apps"),
                QStringLiteral("/usr/share/icons/hicolor/128x128/apps"),
                QStringLiteral("/usr/share/icons/hicolor/256x256/apps"),
                QStringLiteral("/usr/share/icons/hicolor/512x512/apps")
            };
            static const QStringList extensions = {
                QStringLiteral(".png"),
                QStringLiteral(".svg"),
                QStringLiteral(".xpm")
            };
            for (const auto &dir : fallbackDirs) {
                for (const auto &ext : extensions) {
                    const QString candidate = dir + QLatin1Char('/') + id + ext;
                    if (QFileInfo::exists(candidate)) {
                        icon = QIcon(candidate);
                        if (!icon.isNull()) break;
                    }
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
