#include "infrastructure/wayland/WaylandSurfaceMaskSynchronizer.hpp"
#include <QPainterPath>
#include <QRegion>
#include <KWindowEffects>

namespace cachy_dock::infrastructure::wayland {

WaylandSurfaceMaskSynchronizer::WaylandSurfaceMaskSynchronizer(QQuickWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}

WaylandSurfaceMaskSynchronizer::~WaylandSurfaceMaskSynchronizer() = default;

void WaylandSurfaceMaskSynchronizer::updateSurfaceMasks(const QRectF &pillRect, const std::vector<QRectF> &activeIconRects) {
    if (!m_window) {
        return;
    }

    if (pillRect.width() <= 0.0 || pillRect.height() <= 0.0) {
        m_window->setMask(QRegion());
        KWindowEffects::enableBlurBehind(m_window, false);
        return;
    }

    // 1. Build the pill region for KWin blur-behind
    QPainterPath pillPath;
    const double radius = pillRect.height() / 2.0;
    pillPath.addRoundedRect(pillRect, radius, radius);
    const QRegion pillRegion(pillPath.toFillPolygon().toPolygon());

    // Enable hardware dual-kawase blur and macOS-like saturation boost behind pill
    KWindowEffects::enableBlurBehind(m_window, true, pillRegion);
    KWindowEffects::enableBackgroundContrast(m_window, true, 1.2, 1.1, 1.3, pillRegion);

    // 2. Build the input click-through mask
    // Must include the pill + any magnified icon parts protruding above the pill
    QRegion inputRegion = pillRegion;
    for (const auto &iconRect : activeIconRects) {
        if (iconRect.y() < pillRect.y()) {
            // Icon protrudes above the pill, add its bounding box to input region
            inputRegion = inputRegion.united(iconRect.toAlignedRect());
        }
    }

    m_window->setMask(inputRegion);
}

void WaylandSurfaceMaskSynchronizer::applySensorStripMask(double surfaceWidth, double surfaceHeight, double sensorHeight) {
    if (!m_window) {
        return;
    }

    // When auto-hidden, disable blur and collapse input mask to a thin strip at screen bottom
    KWindowEffects::enableBlurBehind(m_window, false);
    const QRect sensorRect(0, static_cast<int>(surfaceHeight - sensorHeight),
                          static_cast<int>(surfaceWidth), static_cast<int>(sensorHeight));
    m_window->setMask(QRegion(sensorRect));
}

} // namespace cachy_dock::infrastructure::wayland
