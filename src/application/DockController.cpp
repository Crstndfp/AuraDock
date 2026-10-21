#include "application/DockController.hpp"
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <algorithm>
#include <chrono>

namespace cachy_dock::application {

static bool matchesAppId(const std::string& itemId, const std::string& windowAppId) {
    if (itemId.empty() || windowAppId.empty()) {
        return false;
    }
    if (itemId == windowAppId) {
        return true;
    }
    QString qItem = QString::fromStdString(itemId);
    QString qWin = QString::fromStdString(windowAppId);

    QString itemBase = QFileInfo(qItem).completeBaseName();
    if (itemBase == qWin || itemBase.endsWith(qWin) || qWin.endsWith(itemBase)) {
        return true;
    }
    return false;
}

DockController::DockController(QObject *parent)
    : QAbstractListModel(parent) {
    
    // Connect bounce animation frame timer (runs at ~120Hz)
    m_bounceTimer.setInterval(8);
    connect(&m_bounceTimer, &QTimer::timeout, this, &DockController::onBounceTick);

    loadSettings();
    applySettingsToEngine();
}

DockController::~DockController() = default;

int DockController::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant DockController::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const auto &item = m_items[index.row()];
    const auto &transform = (index.row() < static_cast<int>(m_lastLayout.iconTransforms.size()))
                                ? m_lastLayout.iconTransforms[index.row()]
                                : domain::services::IconTransform{};

    switch (role) {
        case IdRole:
            return QString::fromStdString(item.id);
        case NameRole:
            return QString::fromStdString(item.name);
        case IconNameRole:
            return QString::fromStdString(item.iconName);
        case TypeRole:
            return static_cast<int>(item.type);
        case IsPinnedRole:
            return item.isPinned;
        case IsRunningRole:
            return item.isRunning;
        case IsFocusedRole:
            return item.isFocused;
        case RunningWindowCountRole:
            return item.runningWindowCount;
        case BounceOffsetRole:
            return item.bounceOffset;
        case ScaleRole:
            return transform.scale;
        case ItemXRole:
            return transform.x;
        case ItemYRole:
            return transform.y - item.bounceOffset;
        case ItemWidthRole:
            return transform.width;
        case ItemHeightRole:
            return transform.height;
        default:
            return {};
    }
}

QHash<int, QByteArray> DockController::roleNames() const {
    return {
        {IdRole, "itemId"},
        {NameRole, "name"},
        {IconNameRole, "iconName"},
        {TypeRole, "itemType"},
        {IsPinnedRole, "isPinned"},
        {IsRunningRole, "isRunning"},
        {IsFocusedRole, "isFocused"},
        {RunningWindowCountRole, "runningWindowCount"},
        {BounceOffsetRole, "bounceOffset"},
        {ScaleRole, "itemScale"},
        {ItemXRole, "itemX"},
        {ItemYRole, "itemY"},
        {ItemWidthRole, "itemWidth"},
        {ItemHeightRole, "itemHeight"}
    };
}

void DockController::setSurfaceWidth(double width) {
    if (qFuzzyCompare(m_surfaceWidth, width)) {
        return;
    }
    m_surfaceWidth = width;
    emit surfaceDimensionsChanged();
    recomputeLayout();
}

void DockController::setSurfaceHeight(double height) {
    if (qFuzzyCompare(m_surfaceHeight, height)) {
        return;
    }
    m_surfaceHeight = height;
    emit surfaceDimensionsChanged();
    recomputeLayout();
}

double DockController::exclusiveZone() const noexcept {
    const double baseDockHeight = m_waveEngine.getConfig().baseIconSize +
                                  (m_waveEngine.getConfig().pillPaddingY * 2.0) + 12.0;
    return m_autoHideSM.getExclusiveZone(baseDockHeight);
}

double DockController::targetYOffset() const noexcept {
    const double slideDistance = m_waveEngine.getConfig().baseIconSize +
                                 (m_waveEngine.getConfig().pillPaddingY * 2.0) + 24.0;
    return m_autoHideSM.getTargetVerticalOffset(slideDistance);
}

bool DockController::isSensorActive() const noexcept {
    return m_autoHideSM.isSensorActive();
}

void DockController::updateCursor(double x, double y) {
    m_cursor = domain::services::Point2D{.x = x, .y = y};
    recomputeLayout();
}

void DockController::clearCursor() {
    if (m_cursor.has_value()) {
        m_cursor.reset();
        recomputeLayout();
    }
}

int DockController::itemAt(double x, double y) const {
    if (m_lastLayout.iconTransforms.empty()) {
        return -1;
    }

    const double bottom = m_lastLayout.pillGeometry.y + m_lastLayout.pillGeometry.height + 8.0;

    for (size_t i = 0; i < m_lastLayout.iconTransforms.size(); ++i) {
        if (i >= m_items.size()) break;

        const auto &tr = m_lastLayout.iconTransforms[i];
        const bool isSep = (m_items[i].type == domain::entities::DockItemType::Separator);

        const double halfSpacing = isSep ? (m_waveEngine.getConfig().separatorSpacing / 2.0)
                                         : (m_waveEngine.getConfig().itemSpacing / 2.0);
        const double left = tr.x - halfSpacing;
        const double right = tr.x + tr.width + halfSpacing;

        const double top = isSep ? m_lastLayout.pillGeometry.y : (tr.y - 12.0);

        if (x >= left && x <= right && y >= top && y <= bottom) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

QVariantMap DockController::getItemDetails(int index) const {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return {};
    }
    const auto &item = m_items[index];
    return {
        {QStringLiteral("id"), QString::fromStdString(item.id)},
        {QStringLiteral("name"), QString::fromStdString(item.name)},
        {QStringLiteral("iconName"), QString::fromStdString(item.iconName)},
        {QStringLiteral("type"), static_cast<int>(item.type)},
        {QStringLiteral("isPinned"), item.isPinned},
        {QStringLiteral("isRunning"), item.isRunning},
        {QStringLiteral("isFocused"), item.isFocused},
        {QStringLiteral("runningWindowCount"), item.runningWindowCount}
    };
}

void DockController::setWindowTracker(domain::interfaces::IWindowTracker* tracker) {
    m_windowTracker = tracker;
    if (!m_windowTracker) {
        return;
    }

    m_windowTracker->setOnWindowsChanged([this]() {
        syncOpenWindows();
    });

    m_windowTracker->setOnActiveWindowChanged([this](const std::string& activeApp) {
        Q_UNUSED(activeApp);
        syncOpenWindows();
    });

    m_windowTracker->setOnMaximizedStateChanged([this](const std::string& screen, bool hasMaximized) {
        Q_UNUSED(screen);
        m_autoHideSM.onMaximizedWindowChanged(hasMaximized);
        emit autoHideStateChanged();
    });

    syncOpenWindows();
}

int DockController::findSeparatorIndex() const {
    // If there is a DocumentStack, place unpinned running apps before the separator preceding it
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].type == domain::entities::DockItemType::DocumentStack) {
            if (i > 0 && m_items[i - 1].type == domain::entities::DockItemType::Separator) {
                return static_cast<int>(i - 1);
            }
            return static_cast<int>(i);
        }
    }
    // Otherwise look for the last separator
    for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
        if (m_items[i].type == domain::entities::DockItemType::Separator) {
            return i;
        }
    }
    return static_cast<int>(m_items.size());
}

void DockController::syncOpenWindows() {
    if (!m_windowTracker) {
        return;
    }

    const auto openWindows = m_windowTracker->getWindows();
    const bool hasMaximized = m_windowTracker->hasMaximizedWindowOnScreen("");
    m_autoHideSM.onMaximizedWindowChanged(hasMaximized);
    emit autoHideStateChanged();

    bool modelChanged = false;

    // 1. Update status of existing pinned and unpinned items
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto &item = m_items[i];
        if (item.type == domain::entities::DockItemType::Separator ||
            item.type == domain::entities::DockItemType::DocumentStack) {
            continue;
        }

        bool running = false;
        bool focused = false;
        int winCount = 0;

        for (const auto &win : openWindows) {
            if (matchesAppId(item.id, win.appId)) {
                running = true;
                if (win.isActive) {
                    focused = true;
                }
                winCount += win.windowCount;
            }
        }

        if (item.isRunning != running || item.isFocused != focused || item.runningWindowCount != winCount) {
            item.isRunning = running;
            item.isFocused = focused;
            item.runningWindowCount = winCount;
            const auto idx = createIndex(static_cast<int>(i), 0);
            emit dataChanged(idx, idx, {IsRunningRole, IsFocusedRole, RunningWindowCountRole});
        }
    }

    // 2. Remove any unpinned items that are no longer running
    for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
        if (!m_items[i].isPinned && !m_items[i].isRunning) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.erase(m_items.begin() + i);
            endRemoveRows();
            modelChanged = true;
        }
    }

    // 3. For any open window that does not correspond to an existing dock item, add it as unpinned running app
    for (const auto &win : openWindows) {
        bool alreadyInDock = false;
        for (const auto &item : m_items) {
            if (matchesAppId(item.id, win.appId)) {
                alreadyInDock = true;
                break;
            }
        }

        if (!alreadyInDock && !win.appId.empty()) {
            domain::entities::DockItem newItem{
                .id = win.appId,
                .name = win.title.empty() ? win.appId : win.title,
                .iconName = win.iconName.empty() ? win.appId : win.iconName,
                .type = domain::entities::DockItemType::AppLauncher,
                .isPinned = false,
                .isRunning = true,
                .isFocused = win.isActive,
                .runningWindowCount = win.windowCount
            };

            const int insertIndex = findSeparatorIndex();
            beginInsertRows(QModelIndex(), insertIndex, insertIndex);
            m_items.insert(m_items.begin() + insertIndex, std::move(newItem));
            endInsertRows();
            modelChanged = true;
        }
    }

    if (modelChanged) {
        recomputeLayout();
    }
}

void DockController::launchItem(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }

    auto &item = m_items[index];
    if (item.type == domain::entities::DockItemType::Separator) {
        return;
    }

    if (item.isRunning && m_windowTracker) {
        // Raise and activate window directly via Wayland TasksModel
        m_windowTracker->activateApp(item.id);
        return;
    }

    // Launch non-running app
    emit itemActivated(QString::fromStdString(item.id));

    // Trigger launch bounce animation
    m_bouncingItemIndex = index;
    m_items[index].isBouncing = true;
    m_bounceElapsed.restart();
    m_bounceTimer.start();
}

void DockController::onBounceTick() {
    if (m_bouncingItemIndex < 0 || m_bouncingItemIndex >= static_cast<int>(m_items.size())) {
        m_bounceTimer.stop();
        return;
    }

    const double elapsedSec = m_bounceElapsed.nsecsElapsed() / 1'000'000'000.0;
    const auto frame = m_bounceSolver.evaluate(elapsedSec);

    m_items[m_bouncingItemIndex].bounceOffset = frame.offsetY;

    const auto idx = createIndex(m_bouncingItemIndex, 0);
    emit dataChanged(idx, idx, {BounceOffsetRole, ItemYRole});

    if (frame.isFinished) {
        m_bounceTimer.stop();
        m_items[m_bouncingItemIndex].bounceOffset = 0.0;
        m_items[m_bouncingItemIndex].isBouncing = false;
        emit dataChanged(idx, idx, {BounceOffsetRole, ItemYRole});
        m_bouncingItemIndex = -1;
    }
}

void DockController::pointerEnteredDock() {
    m_autoHideSM.onPointerEnteredDock();
    emit autoHideStateChanged();
}

void DockController::pointerLeftDock() {
    m_autoHideSM.onPointerLeftDock();
    emit autoHideStateChanged();
}

void DockController::pointerEnteredSensor() {
    m_autoHideSM.onPointerEnteredSensor();
    emit autoHideStateChanged();
}

void DockController::pointerLeftSensor() {
    m_autoHideSM.onPointerLeftSensor();
    emit autoHideStateChanged();
}

void DockController::onHideAnimationFinished() {
    m_autoHideSM.onHideAnimationFinished();
    emit autoHideStateChanged();
}

void DockController::onRevealAnimationFinished() {
    m_autoHideSM.onRevealAnimationFinished();
    emit autoHideStateChanged();
}

void DockController::pinApp(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }
    m_items[index].isPinned = true;
    const auto idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {IsPinnedRole});
}

void DockController::unpinApp(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }
    if (!m_items[index].isRunning) {
        beginRemoveRows(QModelIndex(), index, index);
        m_items.erase(m_items.begin() + index);
        endRemoveRows();
        recomputeLayout();
    } else {
        m_items[index].isPinned = false;
        const auto idx = createIndex(index, 0);
        emit dataChanged(idx, idx, {IsPinnedRole});
    }
}

void DockController::closeItem(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }

    if (m_windowTracker && m_items[index].isRunning) {
        m_windowTracker->closeAllWindowsForApp(m_items[index].id);
    }

    m_items[index].isRunning = false;
    m_items[index].isFocused = false;
    m_items[index].runningWindowCount = 0;

    if (!m_items[index].isPinned) {
        beginRemoveRows(QModelIndex(), index, index);
        m_items.erase(m_items.begin() + index);
        endRemoveRows();
        recomputeLayout();
    } else {
        const auto idx = createIndex(index, 0);
        emit dataChanged(idx, idx, {IsRunningRole, IsFocusedRole, RunningWindowCountRole});
    }
}

void DockController::addItem(domain::entities::DockItem item) {
    beginInsertRows(QModelIndex(), static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
    m_items.push_back(std::move(item));
    endInsertRows();
    recomputeLayout();
}

void DockController::setItems(std::vector<domain::entities::DockItem> items) {
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    recomputeLayout();
}

void DockController::setBottomMargin(double val) {
    if (std::abs(m_settings.bottomMargin - val) > 0.01) {
        m_settings.bottomMargin = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setBarHeight(double val) {
    if (std::abs(m_settings.barHeight - val) > 0.01) {
        m_settings.barHeight = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setIconBottomMargin(double val) {
    if (std::abs(m_settings.iconBottomMargin - val) > 0.01) {
        m_settings.iconBottomMargin = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setIconSize(double val) {
    if (std::abs(m_settings.iconSize - val) > 0.01) {
        m_settings.iconSize = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setItemSpacing(double val) {
    if (std::abs(m_settings.itemSpacing - val) > 0.01) {
        m_settings.itemSpacing = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setSeparatorSpacing(double val) {
    if (std::abs(m_settings.separatorSpacing - val) > 0.01) {
        m_settings.separatorSpacing = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setPillPaddingX(double val) {
    if (std::abs(m_settings.pillPaddingX - val) > 0.01) {
        m_settings.pillPaddingX = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setCornerRadius(double val) {
    if (std::abs(m_settings.cornerRadius - val) > 0.01) {
        m_settings.cornerRadius = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setAutoCornerRadius(bool val) {
    if (m_settings.autoCornerRadius != val) {
        m_settings.autoCornerRadius = val;
        applySettingsToEngine();
        saveSettings();
    }
}

void DockController::setSettingsOpen(bool open) {
    if (m_isSettingsOpen != open) {
        m_isSettingsOpen = open;
        emit isSettingsOpenChanged();
    }
}

void DockController::resetSettingsToDefaults() {
    m_settings = domain::entities::DockSettings{};
    applySettingsToEngine();
    saveSettings();
}

void DockController::insertSeparator(int afterIndex) {
    int targetIndex = 0;
    if (afterIndex < 0) {
        targetIndex = 0;
    } else if (afterIndex >= static_cast<int>(m_items.size())) {
        targetIndex = static_cast<int>(m_items.size());
    } else {
        targetIndex = afterIndex + 1;
    }

    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    domain::entities::DockItem sep{
        .id = "separator_" + std::to_string(timestamp),
        .name = "Separator",
        .iconName = "",
        .type = domain::entities::DockItemType::Separator,
        .isPinned = true,
        .isRunning = false,
        .isFocused = false
    };

    beginInsertRows(QModelIndex(), targetIndex, targetIndex);
    m_items.insert(m_items.begin() + targetIndex, std::move(sep));
    endInsertRows();

    recomputeLayout();
}

void DockController::removeSeparator(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }
    if (m_items[index].type != domain::entities::DockItemType::Separator) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_items.erase(m_items.begin() + index);
    endRemoveRows();

    recomputeLayout();
}

void DockController::applySettingsToEngine() {
    auto config = m_waveEngine.getConfig();
    config.bottomMargin = m_settings.bottomMargin;
    config.baselineOffsetFromBottom = m_settings.bottomMargin;
    config.barHeight = m_settings.barHeight;
    config.iconBottomMargin = m_settings.iconBottomMargin;
    config.baseIconSize = m_settings.iconSize;
    config.itemSpacing = m_settings.itemSpacing;
    config.separatorSpacing = m_settings.separatorSpacing;
    config.pillPaddingX = m_settings.pillPaddingX;
    config.autoCornerRadius = m_settings.autoCornerRadius;
    config.customCornerRadius = m_settings.cornerRadius;
    m_waveEngine.setConfig(config);
    recomputeLayout();
    emit settingsChanged();
}

void DockController::loadSettings() {
    QSettings settings("cachy-dock", "cachy-dock");
    m_settings.bottomMargin = settings.value("appearance/bottomMargin", 12.0).toDouble();
    m_settings.barHeight = settings.value("appearance/barHeight", 64.0).toDouble();
    m_settings.iconBottomMargin = settings.value("appearance/iconBottomMargin", 8.0).toDouble();
    m_settings.iconSize = settings.value("appearance/iconSize", 48.0).toDouble();
    m_settings.itemSpacing = settings.value("appearance/itemSpacing", 10.0).toDouble();
    m_settings.separatorSpacing = settings.value("appearance/separatorSpacing", 12.0).toDouble();
    m_settings.pillPaddingX = settings.value("appearance/pillPaddingX", 19.0).toDouble();
    m_settings.cornerRadius = settings.value("appearance/cornerRadius", 20.0).toDouble();
    m_settings.autoCornerRadius = settings.value("appearance/autoCornerRadius", true).toBool();
}

void DockController::saveSettings() {
    QSettings settings("cachy-dock", "cachy-dock");
    settings.setValue("appearance/bottomMargin", m_settings.bottomMargin);
    settings.setValue("appearance/barHeight", m_settings.barHeight);
    settings.setValue("appearance/iconBottomMargin", m_settings.iconBottomMargin);
    settings.setValue("appearance/iconSize", m_settings.iconSize);
    settings.setValue("appearance/itemSpacing", m_settings.itemSpacing);
    settings.setValue("appearance/separatorSpacing", m_settings.separatorSpacing);
    settings.setValue("appearance/pillPaddingX", m_settings.pillPaddingX);
    settings.setValue("appearance/cornerRadius", m_settings.cornerRadius);
    settings.setValue("appearance/autoCornerRadius", m_settings.autoCornerRadius);
}

void DockController::recomputeLayout() {
    std::vector<bool> isSep(m_items.size(), false);
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].type == domain::entities::DockItemType::Separator) {
            isSep[i] = true;
        }
    }

    m_lastLayout = m_waveEngine.computeLayout(m_items.size(), m_surfaceWidth, m_surfaceHeight, m_cursor, isSep);

    int newHoveredIndex = -1;
    if (m_cursor.has_value()) {
        newHoveredIndex = itemAt(m_cursor->x, m_cursor->y);
    }
    if (m_hoveredItemIndex != newHoveredIndex) {
        m_hoveredItemIndex = newHoveredIndex;
        emit hoveredItemIndexChanged();
    }

    if (!m_items.empty()) {
        const auto topLeft = createIndex(0, 0);
        const auto bottomRight = createIndex(static_cast<int>(m_items.size() - 1), 0);
        emit dataChanged(topLeft, bottomRight, {ScaleRole, ItemXRole, ItemYRole, ItemWidthRole, ItemHeightRole});
    }

    emit layoutChanged();

    const QRectF pillRect(m_lastLayout.pillGeometry.x,
                         m_lastLayout.pillGeometry.y,
                         m_lastLayout.pillGeometry.width,
                         m_lastLayout.pillGeometry.height);

    std::vector<QRectF> activeIconRects;
    activeIconRects.reserve(m_lastLayout.iconTransforms.size());
    for (const auto &tr : m_lastLayout.iconTransforms) {
        activeIconRects.emplace_back(tr.x, tr.y, tr.width, tr.height);
    }

    emit maskUpdateRequired(pillRect, activeIconRects);
}

} // namespace cachy_dock::application
