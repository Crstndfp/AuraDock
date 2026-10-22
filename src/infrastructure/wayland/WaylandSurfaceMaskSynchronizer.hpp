#pragma once

#include <QObject>
#include <QQuickWindow>
#include <QRectF>
#include <vector>

namespace cachy_dock::infrastructure::wayland {

class WaylandSurfaceMaskSynchronizer : public QObject {
    Q_OBJECT

public:
    explicit WaylandSurfaceMaskSynchronizer(QQuickWindow *window, QObject *parent = nullptr);
    ~WaylandSurfaceMaskSynchronizer() override;

public slots:
    void updateSurfaceMasks(const QRectF &pillRect,
                            double cornerRadius,
                            double dockOpacity,
                            bool blurEnabled,
                            bool isPopupOpen,
                            bool isSensorActive,
                            const std::vector<QRectF> &activeIconRects);
    void applySensorStripMask(double surfaceWidth, double surfaceHeight, double sensorHeight = 2.0);

private:
    QQuickWindow *m_window{nullptr};
};

} // namespace cachy_dock::infrastructure::wayland
