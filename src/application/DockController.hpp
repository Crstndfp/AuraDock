#pragma once

#include <QAbstractListModel>
#include <QPointF>
#include <QRectF>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QVariantList>
#include <memory>
#include <vector>

#include "domain/entities/DockItem.hpp"
#include "domain/entities/DockSettings.hpp"
#include "domain/interfaces/IWindowTracker.hpp"
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
    Q_PROPERTY(int hoveredItemIndex READ hoveredItemIndex NOTIFY hoveredItemIndexChanged)

    Q_PROPERTY(double exclusiveZone READ exclusiveZone NOTIFY autoHideStateChanged)
    Q_PROPERTY(double targetYOffset READ targetYOffset NOTIFY autoHideStateChanged)
    Q_PROPERTY(bool isSensorActive READ isSensorActive NOTIFY autoHideStateChanged)

    Q_PROPERTY(double surfaceWidth READ surfaceWidth WRITE setSurfaceWidth NOTIFY surfaceDimensionsChanged)
    Q_PROPERTY(double surfaceHeight READ surfaceHeight WRITE setSurfaceHeight NOTIFY surfaceDimensionsChanged)

    // Configurable Dock Settings
    Q_PROPERTY(double bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY settingsChanged)
    Q_PROPERTY(double barHeight READ barHeight WRITE setBarHeight NOTIFY settingsChanged)
    Q_PROPERTY(double iconBottomMargin READ iconBottomMargin WRITE setIconBottomMargin NOTIFY settingsChanged)
    Q_PROPERTY(double iconSize READ iconSize WRITE setIconSize NOTIFY settingsChanged)
    Q_PROPERTY(double itemSpacing READ itemSpacing WRITE setItemSpacing NOTIFY settingsChanged)
    Q_PROPERTY(double separatorSpacing READ separatorSpacing WRITE setSeparatorSpacing NOTIFY settingsChanged)
    Q_PROPERTY(double pillPaddingX READ pillPaddingX WRITE setPillPaddingX NOTIFY settingsChanged)
    Q_PROPERTY(double cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY settingsChanged)
    Q_PROPERTY(bool autoCornerRadius READ autoCornerRadius WRITE setAutoCornerRadius NOTIFY settingsChanged)
    Q_PROPERTY(int multiInstanceMode READ multiInstanceMode WRITE setMultiInstanceMode NOTIFY settingsChanged)
    Q_PROPERTY(double dockOpacity READ dockOpacity WRITE setDockOpacity NOTIFY settingsChanged)
    Q_PROPERTY(double dockTransparency READ dockTransparency WRITE setDockTransparency NOTIFY settingsChanged)
    Q_PROPERTY(bool blurEnabled READ blurEnabled WRITE setBlurEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool isSettingsOpen READ isSettingsOpen WRITE setSettingsOpen NOTIFY isSettingsOpenChanged)

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
    [[nodiscard]] int hoveredItemIndex() const noexcept { return m_hoveredItemIndex; }

    [[nodiscard]] double exclusiveZone() const noexcept;
    [[nodiscard]] double targetYOffset() const noexcept;
    [[nodiscard]] bool isSensorActive() const noexcept;

    [[nodiscard]] double surfaceWidth() const noexcept { return m_surfaceWidth; }
    void setSurfaceWidth(double width);

    [[nodiscard]] double surfaceHeight() const noexcept { return m_surfaceHeight; }
    void setSurfaceHeight(double height);

    // Settings getters & setters
    [[nodiscard]] double bottomMargin() const noexcept { return m_settings.bottomMargin; }
    void setBottomMargin(double val);

    [[nodiscard]] double barHeight() const noexcept { return m_settings.barHeight; }
    void setBarHeight(double val);

    [[nodiscard]] double iconBottomMargin() const noexcept { return m_settings.iconBottomMargin; }
    void setIconBottomMargin(double val);

    [[nodiscard]] double iconSize() const noexcept { return m_settings.iconSize; }
    void setIconSize(double val);

    [[nodiscard]] double itemSpacing() const noexcept { return m_settings.itemSpacing; }
    void setItemSpacing(double val);

    [[nodiscard]] double separatorSpacing() const noexcept { return m_settings.separatorSpacing; }
    void setSeparatorSpacing(double val);

    [[nodiscard]] double pillPaddingX() const noexcept { return m_settings.pillPaddingX; }
    void setPillPaddingX(double val);

    [[nodiscard]] double cornerRadius() const noexcept { return m_settings.cornerRadius; }
    void setCornerRadius(double val);

    [[nodiscard]] bool autoCornerRadius() const noexcept { return m_settings.autoCornerRadius; }
    void setAutoCornerRadius(bool val);

    [[nodiscard]] int multiInstanceMode() const noexcept { return m_settings.multiInstanceMode; }
    void setMultiInstanceMode(int val);

    [[nodiscard]] double dockOpacity() const noexcept { return m_settings.dockOpacity; }
    void setDockOpacity(double val);

    [[nodiscard]] double dockTransparency() const noexcept { return 1.0 - m_settings.dockOpacity; }
    void setDockTransparency(double val);

    [[nodiscard]] bool blurEnabled() const noexcept { return m_settings.blurEnabled; }
    void setBlurEnabled(bool val);

    [[nodiscard]] bool isSettingsOpen() const noexcept { return m_isSettingsOpen; }
    void setSettingsOpen(bool open);

    // QML Invokables for pointer and interaction
    Q_INVOKABLE void updateCursor(double x, double y);
    Q_INVOKABLE void clearCursor();
    Q_INVOKABLE int itemAt(double x, double y) const;
    Q_INVOKABLE QVariantMap getItemDetails(int index) const;
    Q_INVOKABLE QVariantList getWindowsForItem(int index) const;

    Q_INVOKABLE void launchItem(int index);
    Q_INVOKABLE void activateWindowInstance(qulonglong windowId);
    Q_INVOKABLE void closeWindowInstance(qulonglong windowId);
    Q_INVOKABLE void pointerEnteredDock();
    Q_INVOKABLE void pointerLeftDock();
    Q_INVOKABLE void pointerEnteredSensor();
    Q_INVOKABLE void pointerLeftSensor();
    Q_INVOKABLE void onHideAnimationFinished();
    Q_INVOKABLE void onRevealAnimationFinished();

    // App & Separator Management Invokables
    Q_INVOKABLE void pinApp(int index);
    Q_INVOKABLE void unpinApp(int index);
    Q_INVOKABLE void closeItem(int index);
    Q_INVOKABLE void insertSeparator(int afterIndex);
    Q_INVOKABLE void removeSeparator(int index);
    Q_INVOKABLE bool movePinnedItem(int fromIndex, int toIndex);
    Q_INVOKABLE bool moveItemLeft(int index);
    Q_INVOKABLE bool moveItemRight(int index);
    Q_INVOKABLE bool canMoveItemLeft(int index) const;
    Q_INVOKABLE bool canMoveItemRight(int index) const;
    [[nodiscard]] int maxPinnedIndex() const;
    Q_INVOKABLE void resetSettingsToDefaults();
    Q_INVOKABLE void openSettingsWindow() { setSettingsOpen(true); }
    Q_INVOKABLE void closeSettingsWindow() { setSettingsOpen(false); }
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void loadSettings();
    [[nodiscard]] bool hasSavedItems() const;
    [[nodiscard]] static QString configFilePath();
    static void setConfigPathForTesting(const QString &path);

    // Add items programmatically
    void addItem(domain::entities::DockItem item);
    void setItems(std::vector<domain::entities::DockItem> items);

    // Window Tracker Integration
    void setWindowTracker(domain::interfaces::IWindowTracker* tracker);
    void syncOpenWindows();

signals:
    void layoutChanged();
    void hoveredItemIndexChanged();
    void autoHideStateChanged();
    void surfaceDimensionsChanged();
    void settingsChanged();
    void isSettingsOpenChanged();
    void windowsUpdated();
    void maskUpdateRequired(const QRectF &pillRect, double cornerRadius, double dockOpacity, bool blurEnabled, const std::vector<QRectF> &activeIconRects);
    void itemActivated(const QString &id);

private slots:
    void onBounceTick();

private:
    void recomputeLayout();
    int findUnpinnedInsertIndex() const;
    int findMainSeparatorIndex() const;
    void applySettingsToEngine();

    std::vector<domain::entities::DockItem> m_items;
    domain::interfaces::IWindowTracker* m_windowTracker{nullptr};

    domain::entities::DockSettings m_settings;
    bool m_isSettingsOpen{false};

    domain::services::ParabolicMagnificationEngine m_waveEngine;
    domain::services::BouncePhysicsSolver m_bounceSolver;
    domain::services::SmartAutoHideStateMachine m_autoHideSM;

    domain::services::MagnificationResult m_lastLayout;
    std::optional<domain::services::Point2D> m_cursor;
    int m_hoveredItemIndex{-1};

    double m_surfaceWidth{1920.0};
    double m_surfaceHeight{180.0};

    // Bounce animation state
    QTimer m_bounceTimer;
    QElapsedTimer m_bounceElapsed;
    int m_bouncingItemIndex{-1};
};

} // namespace cachy_dock::application
