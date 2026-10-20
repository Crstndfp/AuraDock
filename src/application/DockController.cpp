#include "application/DockController.hpp"
#include <QDebug>
#include <algorithm>

namespace cachy_dock::application {

DockController::DockController(QObject *parent)
    : QAbstractListModel(parent) {
    
    // Connect bounce animation frame timer (runs at ~120Hz)
    m_bounceTimer.setInterval(8);
    connect(&m_bounceTimer, &QTimer::timeout, this, &DockController::onBounceTick);

    recomputeLayout();
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

    const double baselineY = m_surfaceHeight - m_waveEngine.getConfig().baselineOffsetFromBottom;

    for (size_t i = 0; i < m_lastLayout.iconTransforms.size(); ++i) {
        if (i >= m_items.size()) break;
        if (m_items[i].type == domain::entities::DockItemType::Separator) continue;

        const auto &tr = m_lastLayout.iconTransforms[i];

        const double halfSpacing = m_waveEngine.getConfig().itemSpacing / 2.0;
        const double left = tr.x - halfSpacing;
        const double right = tr.x + tr.width + halfSpacing;

        // Reach from top of icon down to the baseline plus padding
        const double top = tr.y - 12.0;
        const double bottom = baselineY + 16.0;

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

void DockController::launchItem(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        return;
    }

    auto &item = m_items[index];
    emit itemActivated(QString::fromStdString(item.id));

    // Trigger bounce animation if app is not already bouncing
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

    // Notify model of bounce change for that item
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

void DockController::updateWindowStates(const std::string& activeAppId,
                                        const std::vector<std::string>& runningAppIds,
                                        bool hasMaximizedOnScreen) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto &item = m_items[i];
        const bool running = std::find(runningAppIds.begin(), runningAppIds.end(), item.id) != runningAppIds.end();
        const bool focused = (item.id == activeAppId);

        if (item.isRunning != running || item.isFocused != focused) {
            item.isRunning = running;
            item.isFocused = focused;
            const auto idx = createIndex(static_cast<int>(i), 0);
            emit dataChanged(idx, idx, {IsRunningRole, IsFocusedRole});
        }
    }

    m_autoHideSM.onMaximizedWindowChanged(hasMaximizedOnScreen);
    emit autoHideStateChanged();
}

void DockController::recomputeLayout() {
    m_lastLayout = m_waveEngine.computeLayout(m_items.size(), m_surfaceWidth, m_surfaceHeight, m_cursor);

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
