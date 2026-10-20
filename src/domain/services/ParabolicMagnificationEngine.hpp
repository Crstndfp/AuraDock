#pragma once

#include <vector>
#include <optional>
#include <cmath>

namespace cachy_dock::domain::services {

struct Point2D {
    double x{0.0};
    double y{0.0};
};

struct IconTransform {
    double x{0.0};
    double y{0.0};
    double width{48.0};
    double height{48.0};
    double scale{1.0};
};

struct PillGeometry {
    double x{0.0};
    double y{0.0};
    double width{0.0};
    double height{0.0};
    double cornerRadius{20.0};
};

struct MagnificationResult {
    std::vector<IconTransform> iconTransforms;
    PillGeometry pillGeometry;
    bool isMagnified{false};
};

struct ParabolicConfig {
    double baseIconSize{48.0};
    double maxScale{1.7};
    double influenceRadius{160.0};
    double itemSpacing{10.0};
    double pillPaddingX{14.0};
    double pillPaddingY{8.0};
    double baselineOffsetFromBottom{12.0};
    double activeVerticalReach{68.0};
};

class ParabolicMagnificationEngine {
public:
    using Config = ParabolicConfig;

    ParabolicMagnificationEngine();
    explicit ParabolicMagnificationEngine(Config config);

    [[nodiscard]] const Config& getConfig() const noexcept { return m_config; }
    void setConfig(const Config& config) noexcept { m_config = config; }

    [[nodiscard]] MagnificationResult computeLayout(
        size_t itemCount,
        double surfaceWidth,
        double surfaceHeight,
        const std::optional<Point2D>& cursorPosition
    );

    [[nodiscard]] double computeScaleAtDistance(double distance) const noexcept;

    void resetState() noexcept { m_wasMagnified = false; }

private:
    Config m_config;
    bool m_wasMagnified{false};
};

} // namespace cachy_dock::domain::services
