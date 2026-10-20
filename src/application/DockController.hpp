#pragma once

#include <QAbstractListModel>
#include <QPointF>
#include <QRectF>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include <vector>

#include "domain/entities/DockItem.hpp"
#include "domain/services/ParabolicMagnificationEngine.hpp"
#include "domain/services/BouncePhysicsSolver.hpp"
#include "domain/services/SmartAutoHideStateMachine.hpp"

namespace cachy_dock::application {

class DockController : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(double pillX READ pillX NOTIFY layoutChanged)
    Q_PROPERTY(double pillY READ pillY NOTIFY layoutChanged)
    Q_PROPERTY(double pillWidth READ pillWidth NOTIFY layoutChanged)
    Q_PROPERTY(double pillHeight READ pillHeight NOTIFY layoutChanged)
    Q_PROPERTY(double pillRadius READ pillRadius NOTIFY layoutChanged)
    Q_PROPERTY(bool isMagnified READ isMagnified NOTIFY layoutChanged)

    Q_PROPERTY(double exclusiveZone READ exclusiveZone NOTIFY autoHideStateChanged)
    Q_PROPERTY(double targetYOffset READ targetYOffset NOTIFY autoHideStateChanged)
    Q_PROPERTY(bool isSensorActive READ isSensorActive NOTIFY autoHideStateChanged)

    Q_PROPERTY(double surfaceWidth READ surfaceWidth WRITE setSurfaceWidth NOTIFY surfaceDimensionsChanged)
    Q_PROPERTY(double surfaceHeight READ surfaceHeight WRITE setSurfaceHeight NOTIFY surfaceDimensionsChanged)

public:
    enum DockRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        IconNameRole,
        TypeRole,
        IsPinnedRole,
        IsRunningRole,
        IsFocusedRole,
        RunningWindowCountRole,
        BounceOffsetRole,
        ScaleRole,
        ItemXRole,
        ItemYRole,
        ItemWidthRole,
        ItemHeightRole
    };
    Q_ENUM(DockRoles)

    explicit DockController(QObject *parent = nullptr);
    ~DockController() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // Properties getters/setters
    [[nodiscard]] double pillX() const noexcept { return m_lastLayout.pillGeometry.x; }
    [[nodiscard]] double pillY() const noexcept { return m_lastLayout.pillGeometry.y; }
    [[nodiscard]] double pillWidth() const noexcept { return m_lastLayout.pillGeometry.width; }
    [[nodiscard]] double pillHeight() const noexcept { return m_lastLayout.pillGeometry.height; }
    [[nodiscard]] double pillRadius() const noexcept { return m_lastLayout.pillGeometry.cornerRadius; }
    [[nodiscard]] bool isMagnified() const noexcept { return m_lastLayout.isMagnified; }

    [[nodiscard]] double exclusiveZone() const noexcept;
    [[nodiscard]] double targetYOffset() const noexcept;
    [[nodiscard]] bool isSensorActive() const noexcept;

    [[nodiscard]] double surfaceWidth() const noexcept { return m_surfaceWidth; }
    void setSurfaceWidth(double width);

    [[nodiscard]] double surfaceHeight() const noexcept { return m_surfaceHeight; }
    void setSurfaceHeight(double height);

    // QML Invokables for pointer and interaction
    Q_INVOKABLE void updateCursor(double x, double y);
    Q_INVOKABLE void clearCursor();
    Q_INVOKABLE void launchItem(int index);
    Q_INVOKABLE void pointerEnteredDock();
    Q_INVOKABLE void pointerLeftDock();
    Q_INVOKABLE void pointerEnteredSensor();
    Q_INVOKABLE void pointerLeftSensor();
    Q_INVOKABLE void onHideAnimationFinished();
    Q_INVOKABLE void onRevealAnimationFinished();

    // App Management Invokables
    Q_INVOKABLE void pinApp(int index);
    Q_INVOKABLE void unpinApp(int index);
    Q_INVOKABLE void closeItem(int index);

    // Add items programmatically
    void addItem(domain::entities::DockItem item);
    void setItems(std::vector<domain::entities::DockItem> items);

    // Window state updates from tracker
    void updateWindowStates(const std::string& activeAppId,
                            const std::vector<std::string>& runningAppIds,
                            bool hasMaximizedOnScreen);

signals:
    void layoutChanged();
    void autoHideStateChanged();
    void surfaceDimensionsChanged();
    void maskUpdateRequired(const QRectF &pillRect, const std::vector<QRectF> &activeIconRects);
    void itemActivated(const QString &id);

private slots:
    void onBounceTick();

private:
    void recomputeLayout();

    std::vector<domain::entities::DockItem> m_items;
    domain::services::ParabolicMagnificationEngine m_waveEngine;
    domain::services::BouncePhysicsSolver m_bounceSolver;
    domain::services::SmartAutoHideStateMachine m_autoHideSM;

    domain::services::MagnificationResult m_lastLayout;
    std::optional<domain::services::Point2D> m_cursor;

    double m_surfaceWidth{1920.0};
    double m_surfaceHeight{180.0};

    // Bounce animation state
    QTimer m_bounceTimer;
    QElapsedTimer m_bounceElapsed;
    int m_bouncingItemIndex{-1};
};

} // namespace cachy_dock::application
