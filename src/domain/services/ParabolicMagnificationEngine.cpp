#include "domain/services/ParabolicMagnificationEngine.hpp"
#include <algorithm>
#include <utility>
#include <numbers>

namespace cachy_dock::domain::services {

ParabolicMagnificationEngine::ParabolicMagnificationEngine()
    : m_config(ParabolicConfig{}) {}

ParabolicMagnificationEngine::ParabolicMagnificationEngine(Config config)
    : m_config(std::move(config)) {}

double ParabolicMagnificationEngine::computeScaleAtDistance(double distance) const noexcept {
    if (distance >= m_config.influenceRadius || distance < 0.0) {
        return 1.0;
    }
    // Raised-cosine bell curve (smooth C1 derivatives at boundaries 0 and R)
    const double angle = (std::numbers::pi * distance) / (2.0 * m_config.influenceRadius);
    const double cosVal = std::cos(angle);
    return 1.0 + (m_config.maxScale - 1.0) * (cosVal * cosVal);
}

MagnificationResult ParabolicMagnificationEngine::computeLayout(
    size_t itemCount,
    double surfaceWidth,
    double surfaceHeight,
    const std::optional<Point2D>& cursorPosition
) const {
    MagnificationResult result;
    result.iconTransforms.resize(itemCount);

    if (itemCount == 0) {
        result.pillGeometry = PillGeometry{
            .x = surfaceWidth / 2.0,
            .y = surfaceHeight - m_config.baseIconSize - m_config.pillPaddingY * 2.0,
            .width = 0.0,
            .height = m_config.baseIconSize + m_config.pillPaddingY * 2.0,
            .cornerRadius = 20.0
        };
        result.isMagnified = false;
        return result;
    }

    // Step 1: Base unmagnified layout to determine equilibrium centers
    const double unmagnifiedTotalWidth = (itemCount * m_config.baseIconSize) +
                                         ((itemCount - 1) * m_config.itemSpacing);
    const double unmagnifiedStartX = (surfaceWidth - unmagnifiedTotalWidth) / 2.0;

    std::vector<double> baseCenters(itemCount);
    for (size_t i = 0; i < itemCount; ++i) {
        baseCenters[i] = unmagnifiedStartX + (i * (m_config.baseIconSize + m_config.itemSpacing)) +
                         (m_config.baseIconSize / 2.0);
    }

    // Determine if pointer is within active dock interaction area
    bool cursorActive = false;
    double cursorX = 0.0;
    if (cursorPosition.has_value()) {
        const double bottomY = surfaceHeight;
        const double topY = surfaceHeight - m_config.activeVerticalReach;
        if (cursorPosition->y >= topY && cursorPosition->y <= bottomY) {
            cursorActive = true;
            cursorX = cursorPosition->x;
        }
    }

    result.isMagnified = cursorActive;

    // Step 2: Compute scale factors for each icon
    std::vector<double> scales(itemCount, 1.0);
    if (cursorActive) {
        for (size_t i = 0; i < itemCount; ++i) {
            const double distance = std::abs(baseCenters[i] - cursorX);
            scales[i] = computeScaleAtDistance(distance);
        }
    }

    // Step 3: Compute dynamically displaced widths and positions
    std::vector<double> scaledWidths(itemCount);
    double totalScaledWidth = 0.0;
    for (size_t i = 0; i < itemCount; ++i) {
        scaledWidths[i] = m_config.baseIconSize * scales[i];
        totalScaledWidth += scaledWidths[i];
    }
    totalScaledWidth += (itemCount - 1) * m_config.itemSpacing;

    const double startX = (surfaceWidth - totalScaledWidth) / 2.0;
    double currentX = startX;

    const double baselineY = surfaceHeight - m_config.baselineOffsetFromBottom;

    for (size_t i = 0; i < itemCount; ++i) {
        const double width = scaledWidths[i];
        const double height = width;
        const double x = currentX;
        const double y = baselineY - height;

        result.iconTransforms[i] = IconTransform{
            .x = x,
            .y = y,
            .width = width,
            .height = height,
            .scale = scales[i]
        };

        currentX += width + m_config.itemSpacing;
    }

    // Step 4: Glassmorphic pill geometry bounds (anchored at base icon size)
    const double pillHeight = m_config.baseIconSize + (m_config.pillPaddingY * 2.0);
    const double pillY = surfaceHeight - pillHeight - (m_config.baselineOffsetFromBottom / 2.0);
    const double pillX = startX - m_config.pillPaddingX;
    const double pillWidth = totalScaledWidth + (m_config.pillPaddingX * 2.0);

    result.pillGeometry = PillGeometry{
        .x = pillX,
        .y = pillY,
        .width = pillWidth,
        .height = pillHeight,
        .cornerRadius = pillHeight / 2.0
    };

    return result;
}

} // namespace cachy_dock::domain::services
