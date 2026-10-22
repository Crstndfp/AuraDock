#include "infrastructure/wayland/WaylandSurfaceMaskSynchronizer.hpp"
#include <QPainterPath>
#include <QRegion>
#include <KWindowEffects>
#include <QDebug>

namespace cachy_dock::infrastructure::wayland {

WaylandSurfaceMaskSynchronizer::WaylandSurfaceMaskSynchronizer(QQuickWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}

WaylandSurfaceMaskSynchronizer::~WaylandSurfaceMaskSynchronizer() = default;

void WaylandSurfaceMaskSynchronizer::updateSurfaceMasks(
    const QRectF &pillRect,
    double cornerRadius,
    double dockOpacity,
    bool blurEnabled,
    bool isPopupOpen,
    bool isSensorActive,
    const std::vector<QRectF> &activeIconRects)
{
    if (!m_window) {
        return;
    }

    // Always disable artificial background contrast to eliminate golden/amber hue distortion
    KWindowEffects::enableBackgroundContrast(m_window, false);

    // 1. Auto-Hide Sensor State: collapse input region to a thin bottom edge strip
    if (isSensorActive) {
        KWindowEffects::enableBlurBehind(m_window, false);
        const int sHeight = m_window->height() > 0 ? m_window->height() : 280;
        const int sWidth = m_window->width() > 0 ? m_window->width() : 1920;
        const QRect sensorRect(0, sHeight - 4, sWidth, 4);
        m_window->setMask(QRegion(sensorRect));
        return;
    }

    // 2. Popup Open State (Multi-Instance Switcher, Context Menu, Document Stack)
    // Clear mask (empty QRegion) so the full window surface captures input to handle
    // clicking outside the popup to dismiss it.
    if (isPopupOpen) {
        m_window->setMask(QRegion());

        if (pillRect.width() > 0.0 && pillRect.height() > 0.0 && blurEnabled && dockOpacity > 0.01) {
            QPainterPath pillPath;
            const double radius = std::clamp(cornerRadius, 0.0, pillRect.height() / 2.0);
            pillPath.addRoundedRect(pillRect, radius, radius);
            const QRegion pillRegion(pillPath.toFillPolygon().toPolygon());
            KWindowEffects::enableBlurBehind(m_window, true, pillRegion);
        } else {
            KWindowEffects::enableBlurBehind(m_window, false);
        }
        return;
    }

    // 3. Normal Dock State (Resting, Hovering, Magnified):
    // Strictly restrict the Wayland surface input region to the interactive zone of the dock.
    // Unoccupied areas across the bottom of the screen pass 100% of mouse clicks to background windows!
    if (pillRect.width() <= 0.0 || pillRect.height() <= 0.0) {
        m_window->setMask(QRegion(0, 0, 0, 0));
        KWindowEffects::enableBlurBehind(m_window, false);
        return;
    }

    const double padX = 24.0;
    const double approachHeadroom = 80.0;
    const double dockBottomY = static_cast<double>(m_window->height() > 0 ? m_window->height() : (pillRect.y() + pillRect.height() + 20.0));
    const double dockTopY = std::max(0.0, pillRect.y() - approachHeadroom);

    QRectF interactiveRect(pillRect.x() - padX, dockTopY,
                          pillRect.width() + (padX * 2.0), dockBottomY - dockTopY);
    QRegion inputRegion = interactiveRect.toAlignedRect();

    // Union with active icon bounds (including parabolic magnification and vertical bouncing offsets)
    for (const auto &iconRect : activeIconRects) {
        QRectF iconZone(iconRect.x() - 8.0, iconRect.y() - 32.0,
                        iconRect.width() + 16.0, iconRect.height() + 32.0);
        inputRegion = inputRegion.united(iconZone.toAlignedRect());
    }

    m_window->setMask(inputRegion);

    // 4. Hardware Dual-Kawase Blur Behind
    if (!blurEnabled || dockOpacity <= 0.01) {
        KWindowEffects::enableBlurBehind(m_window, false);
        return;
    }

    QPainterPath pillPath;
    const double radius = std::clamp(cornerRadius, 0.0, pillRect.height() / 2.0);
    pillPath.addRoundedRect(pillRect, radius, radius);
    const QRegion pillRegion(pillPath.toFillPolygon().toPolygon());

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
