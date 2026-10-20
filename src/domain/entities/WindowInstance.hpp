#pragma once

#include <string>
#include <cstdint>

namespace cachy_dock::domain::entities {

struct WindowInstance {
    uint64_t id{0};
    std::string appId;
    std::string title;
    std::string iconName;
    bool isMaximized{false};
    bool isMinimized{false};
    bool isFullscreen{false};
    bool isActive{false};
    int windowCount{1};
    std::string screenName;
};

} // namespace cachy_dock::domain::entities
