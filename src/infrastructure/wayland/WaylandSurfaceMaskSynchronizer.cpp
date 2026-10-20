#include "infrastructure/wayland/WaylandSurfaceMaskSynchronizer.hpp"
#include <QPainterPath>
#include <QRegion>
#include <KWindowEffects>
#include <QDebug>

namespace cachy_dock::infrastructure::wayland {

WaylandSurfaceMaskSynchronizer::WaylandSurfaceMaskSynchronizer(QQuickWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}

WaylandSurfaceMaskSynchronizer::~WaylandSurfaceMaskSynchronizer() = default;

void WaylandSurfaceMaskSynchronizer::updateSurfaceMasks(const QRectF &pillRect, const std::vector<QRectF> &activeIconRects) {
    Q_UNUSED(activeIconRects);
    if (!m_window) {
        return;
    }

    if (pillRect.width() <= 0.0 || pillRect.height() <= 0.0) {
        KWindowEffects::enableBlurBehind(m_window, false);
        return;
    }

    // 1. Build the pill region for KWin blur-behind
    QPainterPath pillPath;
    const double radius = pillRect.height() / 2.0;
    pillPath.addRoundedRect(pillRect, radius, radius);
    const QRegion pillRegion(pillPath.toFillPolygon().toPolygon());

    // Enable hardware dual-kawase blur and macOS-like saturation boost strictly behind the pill
    KWindowEffects::enableBlurBehind(m_window, true, pillRegion);
    KWindowEffects::enableBackgroundContrast(m_window, true, 1.25, 1.1, 1.35, pillRegion);
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
