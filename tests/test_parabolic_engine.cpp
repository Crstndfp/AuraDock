#include <gtest/gtest.h>
#include "domain/services/ParabolicMagnificationEngine.hpp"

using namespace cachy_dock::domain::services;

TEST(ParabolicMagnificationEngineTest, WhenNoCursorPresent_AllIconsRemainBaseScale) {
    ParabolicMagnificationEngine engine;
    const size_t itemCount = 5;
    const double surfaceWidth = 1920.0;
    const double surfaceHeight = 180.0;

    auto result = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, std::nullopt);

    EXPECT_FALSE(result.isMagnified);
    ASSERT_EQ(result.iconTransforms.size(), itemCount);

    for (const auto& item : result.iconTransforms) {
        EXPECT_DOUBLE_EQ(item.scale, 1.0);
        EXPECT_DOUBLE_EQ(item.width, 48.0);
        EXPECT_DOUBLE_EQ(item.height, 48.0);
    }

    EXPECT_GT(result.pillGeometry.width, 0.0);
    EXPECT_GT(result.pillGeometry.height, 0.0);
}

TEST(ParabolicMagnificationEngineTest, WhenCursorApproachesFromAbove_GraduallyMagnifiesAndReachesMaxAtIcon) {
    ParabolicMagnificationEngine engine;
    const size_t itemCount = 5;
    const double surfaceWidth = 1920.0;
    const double surfaceHeight = 180.0;

    auto initialResult = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, std::nullopt);
    double middleIconCenterX = initialResult.iconTransforms[2].x + (initialResult.iconTransforms[2].width / 2.0);

    // 1. Cursor above the approach zone (y = 60.0): completely unmagnified
    Point2D cursorFarAbove{.x = middleIconCenterX, .y = 60.0};
    auto resultFarAbove = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, cursorFarAbove);
    EXPECT_FALSE(resultFarAbove.isMagnified);
    EXPECT_DOUBLE_EQ(resultFarAbove.iconTransforms[2].scale, 1.0);

    // 2. Cursor in the approach zone (y = 95.0): gradual intermediate magnification
    Point2D cursorApproaching{.x = middleIconCenterX, .y = 95.0};
    auto resultApproaching = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, cursorApproaching);
    EXPECT_TRUE(resultApproaching.isMagnified);
    EXPECT_GT(resultApproaching.iconTransforms[2].scale, 1.0);
    EXPECT_LT(resultApproaching.iconTransforms[2].scale, 1.7);

    // 3. Cursor reaches top of the icon (y = 120.0): reaches maximum magnification peak
    Point2D cursorOnIcon{.x = middleIconCenterX, .y = 120.0};
    auto resultOnIcon = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, cursorOnIcon);
    EXPECT_TRUE(resultOnIcon.isMagnified);
    EXPECT_NEAR(resultOnIcon.iconTransforms[2].scale, 1.7, 0.001);
    EXPECT_GT(resultOnIcon.iconTransforms[2].scale, resultApproaching.iconTransforms[2].scale);
}

TEST(ParabolicMagnificationEngineTest, WhenCursorDirectlyOverIcon_IconReceivesMaxScale) {
    ParabolicMagnificationEngine engine;
    const size_t itemCount = 5;
    const double surfaceWidth = 1920.0;
    const double surfaceHeight = 180.0;

    auto initialResult = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, std::nullopt);
    double middleIconCenterX = initialResult.iconTransforms[2].x + (initialResult.iconTransforms[2].width / 2.0);

    // Place cursor right over middle icon
    Point2D cursor{.x = middleIconCenterX, .y = surfaceHeight - 30.0};
    auto magnifiedResult = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, cursor);

    EXPECT_TRUE(magnifiedResult.isMagnified);
    EXPECT_NEAR(magnifiedResult.iconTransforms[2].scale, 1.7, 0.001);
    EXPECT_GT(magnifiedResult.iconTransforms[1].scale, 1.0);
    EXPECT_GT(magnifiedResult.iconTransforms[3].scale, 1.0);
    EXPECT_LT(magnifiedResult.iconTransforms[1].scale, magnifiedResult.iconTransforms[2].scale);
}

TEST(ParabolicMagnificationEngineTest, LateralDisplacementPushesNeighborsOutward) {
    ParabolicMagnificationEngine engine;
    const size_t itemCount = 7;
    const double surfaceWidth = 1920.0;
    const double surfaceHeight = 180.0;

    auto unmagnified = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, std::nullopt);

    double middleX = unmagnified.iconTransforms[3].x + (unmagnified.iconTransforms[3].width / 2.0);
    Point2D cursor{.x = middleX, .y = surfaceHeight - 20.0};
    auto magnified = engine.computeLayout(itemCount, surfaceWidth, surfaceHeight, cursor);

    // Pill width must expand to enclose the enlarged icons
    EXPECT_GT(magnified.pillGeometry.width, unmagnified.pillGeometry.width);

    // Left neighbor must be shifted to the left
    EXPECT_LT(magnified.iconTransforms[0].x, unmagnified.iconTransforms[0].x);

    // Right neighbor must be shifted to the right
    EXPECT_GT(magnified.iconTransforms[6].x, unmagnified.iconTransforms[6].x);
}

TEST(ParabolicMagnificationEngineTest, ScaleAtBoundaryEqualsOneWithContinuousDerivative) {
    ParabolicMagnificationEngine engine;
    const double radius = engine.getConfig().influenceRadius;

    // Scale at distance >= radius must strictly equal 1.0
    EXPECT_DOUBLE_EQ(engine.computeScaleAtDistance(radius), 1.0);
    EXPECT_DOUBLE_EQ(engine.computeScaleAtDistance(radius + 10.0), 1.0);

    // Scale at 0 must equal maxScale (1.7)
    EXPECT_NEAR(engine.computeScaleAtDistance(0.0), 1.7, 0.0001);

    // Derivative check near boundary: scale at (radius - 1) should be very close to 1
    double nearBoundaryScale = engine.computeScaleAtDistance(radius - 1.0);
    EXPECT_NEAR(nearBoundaryScale, 1.0, 0.001);
}
