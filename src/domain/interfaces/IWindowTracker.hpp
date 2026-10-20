#pragma once

#include "domain/entities/WindowInstance.hpp"
#include <vector>
#include <functional>
#include <string>

namespace cachy_dock::domain::interfaces {

class IWindowTracker {
public:
    virtual ~IWindowTracker() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual std::vector<entities::WindowInstance> getWindows() const = 0;
    virtual bool hasMaximizedWindowOnScreen(const std::string& screenName) const = 0;

    virtual void activateWindow(uint64_t windowId) = 0;
    virtual void activateApp(const std::string& appId) = 0;
    virtual void closeWindow(uint64_t windowId) = 0;
    virtual void closeAllWindowsForApp(const std::string& appId) = 0;

    virtual void setOnWindowsChanged(std::function<void()> callback) = 0;
    virtual void setOnActiveWindowChanged(std::function<void(const std::string& activeAppId)> callback) = 0;
    virtual void setOnMaximizedStateChanged(std::function<void(const std::string& screenName, bool hasMaximized)> callback) = 0;
};

} // namespace cachy_dock::domain::interfaces
