#include "infrastructure/wayland/WaylandSurfaceMaskSynchronizer.hpp"
#include <QPainterPath>
#include <QRegion>
#include <KWindowEffects>
#include <QDebug>

namespace cachy_dock::infrastructure::wayland {

WaylandSurfaceMaskSynchronizer::WaylandSurfaceMaskSynchronizer(QQuickWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}

WaylandSurfaceMaskSynchronizer::~WaylandSurfaceMaskSynchronizer() = default;

void WaylandSurfaceMaskSynchronizer::updateSurfaceMasks(const QRectF &pillRect, double cornerRadius, double dockOpacity, bool blurEnabled, const std::vector<QRectF> &activeIconRects) {
    Q_UNUSED(activeIconRects);
    if (!m_window) {
        return;
    }

    // Always disable artificial background contrast to eliminate golden/amber hue distortion
    KWindowEffects::enableBackgroundContrast(m_window, false);

    if (pillRect.width() <= 0.0 || pillRect.height() <= 0.0 || !blurEnabled || dockOpacity <= 0.01) {
        KWindowEffects::enableBlurBehind(m_window, false);
        return;
    }

    // 1. Build the pill region for KWin blur-behind matching the exact dynamic cornerRadius
    QPainterPath pillPath;
    const double radius = std::clamp(cornerRadius, 0.0, pillRect.height() / 2.0);
    pillPath.addRoundedRect(pillRect, radius, radius);
    const QRegion pillRegion(pillPath.toFillPolygon().toPolygon());

    // Enable clean hardware dual-kawase blur strictly behind the pill
    KWindowEffects::enableBlurBehind(m_window, true, pillRegion);
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
