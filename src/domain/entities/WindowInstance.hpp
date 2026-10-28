#pragma once

#include <string>
#include <cstdint>

namespace cachy_dock::domain::entities {

struct WindowInstance {
    uint64_t id{0};
    std::string appId{};
    std::string title{};
    std::string iconName{};
    bool isMaximized{false};
    bool isMinimized{false};
    bool isFullscreen{false};
    bool isActive{false};
    int windowCount{1};
    std::string screenName{};
    std::string waylandUuid{};
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    int screenX{0};
    int screenY{0};
    int screenWidth{0};
    int screenHeight{0};
};

} // namespace cachy_dock::domain::entities
