#pragma once

namespace cachy_dock::domain::entities {

enum class MultiInstanceMode : int {
    MiniWindows = 0, // Horizontal mini-window preview cards
    WindowList = 1   // Compact vertical window list
};

struct DockSettings {
    double bottomMargin{12.0};     // Margin from screen bottom edge
    double barHeight{64.0};        // Dedicated height of the dock pill
    double iconBottomMargin{8.0};  // Distance between icon bottoms and pill bottom edge
    double iconSize{48.0};          // Base icon size
    double itemSpacing{10.0};       // Gap between adjacent icons
    double separatorSpacing{12.0}; // Horizontal gap to left and right of separators
    double pillPaddingX{19.0};     // End padding (+5px extra breathing room at rounded curves)
    double cornerRadius{20.0};      // Custom pill corner curve radius
    bool autoCornerRadius{true};    // If true, cornerRadius = pillHeight / 2.0 (pill shape)
    int multiInstanceMode{0};       // 0 = MiniWindows, 1 = WindowList
    double dockOpacity{0.42};       // Dock glass background opacity (0.0 = fully transparent glass, 1.0 = solid)
    bool blurEnabled{true};         // Enable hardware real-time blur behind the dock
};

} // namespace cachy_dock::domain::entities
