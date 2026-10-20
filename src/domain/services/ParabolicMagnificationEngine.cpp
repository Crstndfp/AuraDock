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
) {
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
        m_wasMagnified = false;
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

    // Step 2: Determine if pointer is directly on the icon/dock
    const double baselineY = surfaceHeight - m_config.baselineOffsetFromBottom;
    const double unmagnifiedIconTopY = baselineY - m_config.baseIconSize; // e.g. 168 - 48 = 120.0
    const double maxMagnifiedIconTopY = baselineY - (m_config.baseIconSize * m_config.maxScale); // e.g. 168 - 81.6 = 86.4

    const double pillLeft = unmagnifiedStartX - m_config.pillPaddingX;
    const double pillRight = unmagnifiedStartX + unmagnifiedTotalWidth + m_config.pillPaddingX;

    bool cursorActive = false;
    double cursorX = 0.0;

    if (cursorPosition.has_value()) {
        const double cx = cursorPosition->x;
        const double cy = cursorPosition->y;

        // Horizontally within the dock pill range (with small tolerance)
        const double currentPillLeft = m_wasMagnified ? (pillLeft - 30.0) : (pillLeft - 8.0);
        const double currentPillRight = m_wasMagnified ? (pillRight + 30.0) : (pillRight + 8.0);

        if (cx >= currentPillLeft && cx <= currentPillRight && cy <= (surfaceHeight + 20.0)) {
            if (m_wasMagnified) {
                // Once magnified, the active vertical region extends up to the top of the expanded icon
                if (cy >= (maxMagnifiedIconTopY - 4.0)) {
                    cursorActive = true;
                    cursorX = cx;
                }
            } else {
                // When unmagnified, ONLY activate when the cursor is directly on the icon/pill top
                // (no activation 20px above the icon)
                if (cy >= (unmagnifiedIconTopY - 2.0)) {
                    cursorActive = true;
                    cursorX = cx;
                }
            }
        }
    }

    m_wasMagnified = cursorActive;
    result.isMagnified = cursorActive;

    // Step 3: Compute scale factors for each icon
    std::vector<double> scales(itemCount, 1.0);
    if (cursorActive) {
        for (size_t i = 0; i < itemCount; ++i) {
            const double distance = std::abs(baseCenters[i] - cursorX);
            scales[i] = computeScaleAtDistance(distance);
        }
    }

    // Step 4: Compute dynamically displaced widths and positions
    std::vector<double> scaledWidths(itemCount);
    double totalScaledWidth = 0.0;
    for (size_t i = 0; i < itemCount; ++i) {
        scaledWidths[i] = m_config.baseIconSize * scales[i];
        totalScaledWidth += scaledWidths[i];
    }
    totalScaledWidth += (itemCount - 1) * m_config.itemSpacing;

    const double startX = (surfaceWidth - totalScaledWidth) / 2.0;
    double currentX = startX;

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

    // Step 5: Glassmorphic pill geometry bounds (anchored at base icon size)
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
