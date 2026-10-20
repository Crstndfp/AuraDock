#include "infrastructure/trackers/KWinWaylandTracker.hpp"
#include <QDBusMessage>
#include <QDebug>
#include <algorithm>

namespace cachy_dock::infrastructure::trackers {

KWinWaylandTracker::KWinWaylandTracker(QObject *parent)
    : QObject(parent) {}

KWinWaylandTracker::~KWinWaylandTracker() {
    stop();
}

void KWinWaylandTracker::start() {
    m_isRunning = true;
}

void KWinWaylandTracker::stop() {
    m_isRunning = false;
}

std::vector<domain::entities::WindowInstance> KWinWaylandTracker::getWindows() const {
    return m_windows;
}

bool KWinWaylandTracker::hasMaximizedWindowOnScreen(const std::string& screenName) const {
    Q_UNUSED(screenName);
    return m_hasMaximizedOnCurrentScreen;
}

void KWinWaylandTracker::activateWindow(uint64_t windowId) {
    for (auto &win : m_windows) {
        win.isActive = (win.id == windowId);
    }
    if (m_onActiveWindowChanged) {
        m_onActiveWindowChanged(windowId);
    }
}

void KWinWaylandTracker::closeWindow(uint64_t windowId) {
    auto it = std::remove_if(m_windows.begin(), m_windows.end(), [windowId](const auto &win) {
        return win.id == windowId;
    });
    if (it != m_windows.end()) {
        m_windows.erase(it, m_windows.end());
        if (m_onWindowsChanged) {
            m_onWindowsChanged();
        }
    }
}

void KWinWaylandTracker::closeAllWindowsForApp(const std::string& appId) {
    auto it = std::remove_if(m_windows.begin(), m_windows.end(), [&appId](const auto &win) {
        return win.appId == appId;
    });
    if (it != m_windows.end()) {
        m_windows.erase(it, m_windows.end());
        if (m_onWindowsChanged) {
            m_onWindowsChanged();
        }
    }
}

void KWinWaylandTracker::setOnWindowsChanged(std::function<void()> callback) {
    m_onWindowsChanged = std::move(callback);
}

void KWinWaylandTracker::setOnActiveWindowChanged(std::function<void(uint64_t)> callback) {
    m_onActiveWindowChanged = std::move(callback);
}

void KWinWaylandTracker::setOnMaximizedStateChanged(std::function<void(const std::string& screenName, bool hasMaximized)> callback) {
    m_onMaximizedStateChanged = std::move(callback);
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
