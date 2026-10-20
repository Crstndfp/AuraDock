#pragma once

#include <string>

namespace cachy_dock::domain::entities {

enum class DockItemType {
    AppLauncher,
    RecentApp,
    DocumentStack,
    Separator
};

struct DockItem {
    std::string id;
    std::string name;
    std::string iconName;
    DockItemType type{DockItemType::AppLauncher};
    bool isPinned{true};
    bool isRunning{false};
    bool isFocused{false};
    int runningWindowCount{0};
    bool isBouncing{false};
    double bounceOffset{0.0};
    double scale{1.0};
    double x{0.0};
    double y{0.0};
    double width{48.0};
    double height{48.0};
};

} // namespace cachy_dock::domain::entities
