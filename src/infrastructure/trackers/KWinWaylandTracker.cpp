#include "infrastructure/trackers/KWinWaylandTracker.hpp"
#include <taskmanager/tasksmodel.h>
#include <taskmanager/abstracttasksmodel.h>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>
#include <algorithm>

namespace cachy_dock::infrastructure::trackers {

static QString normalizeAppId(const QString &raw) {
    if (raw.isEmpty()) return QString();
    QString s = raw;
    if (s.startsWith(QStringLiteral("file://"))) {
        s = QUrl(s).toLocalFile();
    } else if (s.startsWith(QStringLiteral("applications:"))) {
        s = s.mid(13);
    }
    s = QFileInfo(s).fileName();
    if (s.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive)) {
        s.chop(8);
    }
    return s.trimmed().toLower();
}

static bool isAppMatching(const QString &appA, const QString &appB) {
    if (appA.isEmpty() || appB.isEmpty()) return false;
    const QString normA = normalizeAppId(appA);
    const QString normB = normalizeAppId(appB);
    if (normA == normB) return true;

    const QString lastA = normA.section(QLatin1Char('.'), -1);
    const QString lastB = normB.section(QLatin1Char('.'), -1);
    if (!lastA.isEmpty() && lastA == normB) return true;
    if (!lastB.isEmpty() && lastB == normA) return true;
    if (!lastA.isEmpty() && !lastB.isEmpty() && lastA == lastB) return true;

    return false;
}

KWinWaylandTracker::KWinWaylandTracker(QObject *parent)
    : QObject(parent) {}

KWinWaylandTracker::~KWinWaylandTracker() {
    stop();
}

void KWinWaylandTracker::start() {
    if (m_isRunning) {
        return;
    }
    m_isRunning = true;

    m_tasksModel = new TaskManager::TasksModel(this);

    connect(m_tasksModel, &QAbstractItemModel::rowsInserted, this, &KWinWaylandTracker::onTasksUpdated);
    connect(m_tasksModel, &QAbstractItemModel::rowsRemoved, this, &KWinWaylandTracker::onTasksUpdated);
    connect(m_tasksModel, &QAbstractItemModel::dataChanged, this, &KWinWaylandTracker::onTasksUpdated);
    connect(m_tasksModel, &QAbstractItemModel::modelReset, this, &KWinWaylandTracker::onTasksUpdated);

    onTasksUpdated();
}

void KWinWaylandTracker::stop() {
    m_isRunning = false;
    if (m_tasksModel) {
        delete m_tasksModel;
        m_tasksModel = nullptr;
    }
}

std::vector<domain::entities::WindowInstance> KWinWaylandTracker::getWindows() const {
    return m_windows;
}

bool KWinWaylandTracker::hasMaximizedWindowOnScreen(const std::string& screenName) const {
    Q_UNUSED(screenName);
    return m_hasMaximizedOnCurrentScreen;
}

void KWinWaylandTracker::activateWindow(uint64_t windowId) {
    if (m_tasksModel && windowId < static_cast<uint64_t>(m_tasksModel->rowCount())) {
        const auto idx = m_tasksModel->index(static_cast<int>(windowId), 0);
        m_tasksModel->requestActivate(idx);
    }
}

void KWinWaylandTracker::activateApp(const std::string& appId) {
    if (!m_tasksModel) {
        return;
    }

    const QString target = QString::fromStdString(appId);

    for (int r = 0; r < m_tasksModel->rowCount(); ++r) {
        const auto idx = m_tasksModel->index(r, 0);
        const QString rowAppId = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::AppId).toString();
        const QString launcherUrl = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::LauncherUrlWithoutIcon).toString();

        if (isAppMatching(target, rowAppId) || isAppMatching(target, launcherUrl)) {
            const bool isMinimized = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::IsMinimized).toBool();
            if (isMinimized) {
                m_tasksModel->requestToggleMinimized(idx);
            }
            m_tasksModel->requestActivate(idx);
            return;
        }
    }
}

void KWinWaylandTracker::closeWindow(uint64_t windowId) {
    if (m_tasksModel && windowId < static_cast<uint64_t>(m_tasksModel->rowCount())) {
        const auto idx = m_tasksModel->index(static_cast<int>(windowId), 0);
        m_tasksModel->requestClose(idx);
    }
}

void KWinWaylandTracker::closeAllWindowsForApp(const std::string& appId) {
    if (!m_tasksModel) {
        return;
    }

    const QString target = QString::fromStdString(appId);

    for (int r = 0; r < m_tasksModel->rowCount(); ++r) {
        const auto idx = m_tasksModel->index(r, 0);
        const QString rowAppId = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::AppId).toString();
        const QString launcherUrl = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::LauncherUrlWithoutIcon).toString();

        if (isAppMatching(target, rowAppId) || isAppMatching(target, launcherUrl)) {
            m_tasksModel->requestClose(idx);
        }
    }
}

void KWinWaylandTracker::setOnWindowsChanged(std::function<void()> callback) {
    m_onWindowsChanged = std::move(callback);
}

void KWinWaylandTracker::setOnActiveWindowChanged(std::function<void(const std::string&)> callback) {
    m_onActiveWindowChanged = std::move(callback);
}

void KWinWaylandTracker::setOnMaximizedStateChanged(std::function<void(const std::string&, bool)> callback) {
    m_onMaximizedStateChanged = std::move(callback);
}

void KWinWaylandTracker::onTasksUpdated() {
    if (!m_tasksModel) {
        return;
    }

    std::vector<domain::entities::WindowInstance> windows;
    bool hasMaximized = false;
    std::string activeApp;

    for (int r = 0; r < m_tasksModel->rowCount(); ++r) {
        const auto idx = m_tasksModel->index(r, 0);

        const QString appId = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::AppId).toString();
        const QString appName = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::AppName).toString();
        const QString genericName = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::GenericName).toString();
        const bool isActive = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::IsActive).toBool();
        const bool isMaximized = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::IsMaximized).toBool();
        const bool isMinimized = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::IsMinimized).toBool();
        const bool isFullscreen = m_tasksModel->data(idx, TaskManager::AbstractTasksModel::IsFullScreen).toBool();
        const int childCount = std::max(1, m_tasksModel->data(idx, TaskManager::AbstractTasksModel::ChildCount).toInt());

        if (isMaximized || isFullscreen) {
            hasMaximized = true;
        }

        if (isActive) {
            activeApp = appId.toStdString();
        }

        domain::entities::WindowInstance win{
            .id = static_cast<uint64_t>(r),
            .appId = appId.toStdString(),
            .title = appName.isEmpty() ? (genericName.isEmpty() ? appId.toStdString() : genericName.toStdString()) : appName.toStdString(),
            .iconName = appId.toStdString(),
            .isMaximized = isMaximized,
            .isMinimized = isMinimized,
            .isFullscreen = isFullscreen,
            .isActive = isActive,
            .windowCount = childCount,
            .screenName = ""
        };
        windows.push_back(std::move(win));
    }

    m_windows = std::move(windows);
    m_hasMaximizedOnCurrentScreen = hasMaximized;
    m_activeAppId = activeApp;

    if (m_onWindowsChanged) {
        m_onWindowsChanged();
    }
    if (m_onActiveWindowChanged) {
        m_onActiveWindowChanged(m_activeAppId);
    }
    if (m_onMaximizedStateChanged) {
        m_onMaximizedStateChanged("", m_hasMaximizedOnCurrentScreen);
    }
}

void KWinWaylandTracker::simulateWindowChange(const std::vector<domain::entities::WindowInstance>& windows) {
    m_windows = windows;
    if (m_onWindowsChanged) {
        m_onWindowsChanged();
    }
}

void KWinWaylandTracker::simulateMaximizedState(const std::string& screenName, bool hasMaximized) {
    m_hasMaximizedOnCurrentScreen = hasMaximized;
    if (m_onMaximizedStateChanged) {
        m_onMaximizedStateChanged(screenName, hasMaximized);
    }
}

} // namespace cachy_dock::infrastructure::trackers
