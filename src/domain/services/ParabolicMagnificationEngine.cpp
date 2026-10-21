#include "domain/services/ParabolicMagnificationEngine.hpp"
#include <algorithm>
#include <utility>
#include <numbers>

namespace cachy_dock::domain::services {

ParabolicMagnificationEngine::ParabolicMagnificationEngine()
    : m_config(ParabolicConfig{}) {}

ParabolicMagnificationEngine::ParabolicMagnificationEngine(Config config)
    : m_config(std::move(config)) {}

double ParabolicMagnificationEngine::computeHorizontalFactorAtDistance(double distance) const noexcept {
    if (distance >= m_config.influenceRadius || distance < 0.0) {
        return 0.0;
    }
    // Raised-cosine bell curve (smooth C1 derivatives at boundaries 0 and R)
    const double angle = (std::numbers::pi * distance) / (2.0 * m_config.influenceRadius);
    const double cosVal = std::cos(angle);
    return cosVal * cosVal;
}

double ParabolicMagnificationEngine::computeScaleAtDistance(double distance) const noexcept {
    return 1.0 + (m_config.maxScale - 1.0) * computeHorizontalFactorAtDistance(distance);
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
        const double effectiveBottomMargin = m_config.bottomMargin;
        const double pillHeight = m_config.barHeight;
        result.pillGeometry = PillGeometry{
            .x = surfaceWidth / 2.0,
            .y = surfaceHeight - effectiveBottomMargin - pillHeight,
            .width = 0.0,
            .height = pillHeight,
            .cornerRadius = pillHeight / 2.0
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

    // Step 2: Compute smooth vertical approach factor and unified vertical metrics
    const double effectiveBottomMargin = m_config.bottomMargin;
    const double pillHeight = m_config.barHeight;
    const double pillBottomY = surfaceHeight - effectiveBottomMargin;
    const double pillY = pillBottomY - pillHeight;

    // Center unmagnified icons vertically within the dock pill
    const double verticalPadding = std::max(4.0, (pillHeight - m_config.baseIconSize) / 2.0);
    const double baselineY = pillBottomY - verticalPadding;
    const double unmagnifiedIconTopY = baselineY - m_config.baseIconSize;
    const double approachStartY = unmagnifiedIconTopY - m_config.approachDistance;

    const double pillLeft = unmagnifiedStartX - m_config.pillPaddingX - m_config.influenceRadius;
    const double pillRight = unmagnifiedStartX + unmagnifiedTotalWidth + m_config.pillPaddingX + m_config.influenceRadius;

    bool cursorActive = false;
    double cursorX = 0.0;
    double verticalFactor = 0.0;

    if (cursorPosition.has_value()) {
        const double cx = cursorPosition->x;
        const double cy = cursorPosition->y;

        if (cx >= pillLeft && cx <= pillRight && cy >= approachStartY && cy <= (surfaceHeight + 20.0)) {
            if (cy >= unmagnifiedIconTopY) {
                // When cursor is on or below the top of the icon, magnification is at its maximum peak
                verticalFactor = 1.0;
            } else {
                // Gradual approach: smoothstep ramp from approachStartY to unmagnifiedIconTopY
                const double t = (cy - approachStartY) / m_config.approachDistance;
                verticalFactor = t * t * (3.0 - 2.0 * t);
            }
            cursorActive = (verticalFactor > 0.005);
            cursorX = cx;
        }
    }

    m_wasMagnified = cursorActive;
    result.isMagnified = cursorActive;

    // Step 3: Compute scale factors for each icon using combined horizontal + vertical gradients
    std::vector<double> scales(itemCount, 1.0);
    if (cursorActive) {
        for (size_t i = 0; i < itemCount; ++i) {
            const double distance = std::abs(baseCenters[i] - cursorX);
            const double horizontalFactor = computeHorizontalFactorAtDistance(distance);
            scales[i] = 1.0 + (m_config.maxScale - 1.0) * horizontalFactor * verticalFactor;
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

    // Step 5: Glassmorphic pill geometry bounds
    const double pillX = startX - m_config.pillPaddingX;
    const double pillWidth = totalScaledWidth + (m_config.pillPaddingX * 2.0);

    const double cornerRadius = m_config.autoCornerRadius
        ? (pillHeight / 2.0)
        : std::clamp(m_config.customCornerRadius, 0.0, pillHeight / 2.0);

    result.pillGeometry = PillGeometry{
        .x = pillX,
        .y = pillY,
        .width = pillWidth,
        .height = pillHeight,
        .cornerRadius = cornerRadius
    };

    return result;
}

} // namespace cachy_dock::domain::services
