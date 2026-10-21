#pragma once

namespace cachy_dock::domain::entities {

struct DockSettings {
    double bottomMargin{12.0};     // Margin from screen bottom edge (baselineOffsetFromBottom)
    double iconSize{48.0};          // Base icon size (controls dock height)
    double itemSpacing{10.0};       // Gap between adjacent icons / separators
    double cornerRadius{20.0};      // Custom pill corner curve radius
    bool autoCornerRadius{true};    // If true, cornerRadius = pillHeight / 2.0 (pill shape)
};

} // namespace cachy_dock::domain::entities
