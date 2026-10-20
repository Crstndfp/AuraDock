#pragma once

#include <QQuickImageProvider>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QColor>

namespace cachy_dock::infrastructure::xdg {

class ThemeIconProvider : public QQuickImageProvider {
public:
    ThemeIconProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override {
        const int width = requestedSize.width() > 0 ? requestedSize.width() : 128;
        const int height = requestedSize.height() > 0 ? requestedSize.height() : 128;

        if (size) {
            *size = QSize(width, height);
        }

        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull()) {
            icon = QIcon(id);
        }

        if (!icon.isNull()) {
            QPixmap px = icon.pixmap(width, height);
            if (!px.isNull()) {
                return px;
            }
        }

        // Generate a high-DPI glassmorphic placeholder icon badge
        QPixmap fallback(width, height);
        fallback.fill(Qt::transparent);

        QPainter painter(&fallback);
        painter.setRenderHint(QPainter::Antialiasing);

        // Compute stable color from id hash
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
};

} // namespace cachy_dock::infrastructure::xdg
