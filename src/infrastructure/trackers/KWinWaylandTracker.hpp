#pragma once

#include "domain/interfaces/IWindowTracker.hpp"
#include <QObject>
#include <vector>
#include <string>

namespace TaskManager {
class TasksModel;
}

namespace cachy_dock::infrastructure::trackers {

class KWinWaylandTracker : public QObject, public domain::interfaces::IWindowTracker {
    Q_OBJECT

public:
    explicit KWinWaylandTracker(QObject *parent = nullptr);
    ~KWinWaylandTracker() override;

    void start() override;
    void stop() override;

    [[nodiscard]] std::vector<domain::entities::WindowInstance> getWindows() const override;
    [[nodiscard]] bool hasMaximizedWindowOnScreen(const std::string& screenName) const override;

    void activateWindow(uint64_t windowId) override;
    void activateApp(const std::string& appId) override;
    void closeWindow(uint64_t windowId) override;
    void closeAllWindowsForApp(const std::string& appId) override;

    void setOnWindowsChanged(std::function<void()> callback) override;
    void setOnActiveWindowChanged(std::function<void(const std::string& activeAppId)> callback) override;
    void setOnMaximizedStateChanged(std::function<void(const std::string& screenName, bool hasMaximized)> callback) override;

public slots:
    void simulateWindowChange(const std::vector<domain::entities::WindowInstance>& windows);
    void simulateMaximizedState(const std::string& screenName, bool hasMaximized);

private slots:
    void onTasksUpdated();

private:
    TaskManager::TasksModel *m_tasksModel{nullptr};
    std::vector<domain::entities::WindowInstance> m_windows;
    bool m_isRunning{false};
    bool m_hasMaximizedOnCurrentScreen{false};
    std::string m_activeAppId;

    std::function<void()> m_onWindowsChanged;
    std::function<void(const std::string&)> m_onActiveWindowChanged;
    std::function<void(const std::string&, bool)> m_onMaximizedStateChanged;
};

} // namespace cachy_dock::infrastructure::trackers
