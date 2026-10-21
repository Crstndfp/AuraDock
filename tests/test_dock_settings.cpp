#include <gtest/gtest.h>
#include "application/DockController.hpp"
#include "domain/entities/DockItem.hpp"

using namespace cachy_dock::application;
using namespace cachy_dock::domain::entities;

class DockSettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::vector<DockItem> items = {
            DockItem{.id = "dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
            DockItem{.id = "firefox.desktop", .name = "Firefox", .iconName = "firefox", .type = DockItemType::AppLauncher, .isPinned = true},
            DockItem{.id = "separator", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true},
            DockItem{.id = "downloads", .name = "Downloads", .iconName = "folder-download", .type = DockItemType::DocumentStack, .isPinned = true}
        };
        controller.resetSettingsToDefaults();
        controller.setItems(items);
    }

    DockController controller;
};

TEST_F(DockSettingsTest, ChangingBottomMarginShiftsIconsAndPillInLockstep) {
    const double initialPillY = controller.pillY();
    const double initialIconY = controller.data(controller.index(0, 0), DockController::ItemYRole).toDouble();
    const double delta = 25.0;

    // Increase bottom margin by 25px
    controller.setBottomMargin(controller.bottomMargin() + delta);

    const double newPillY = controller.pillY();
    const double newIconY = controller.data(controller.index(0, 0), DockController::ItemYRole).toDouble();

    // Both pill and icons MUST shift upwards by the EXACT same delta (1:1 lockstep)
    EXPECT_NEAR(initialPillY - newPillY, delta, 0.001);
    EXPECT_NEAR(initialIconY - newIconY, delta, 0.001);
    EXPECT_NEAR((initialIconY - initialPillY), (newIconY - newPillY), 0.001);
}

TEST_F(DockSettingsTest, ChangingBarHeightAltersPillHeightWithoutIncreasingDockWidth) {
    const double initialBarHeight = controller.barHeight();
    const double initialPillHeight = controller.pillHeight();
    const double initialPillWidth = controller.pillWidth();
    const double initialIconWidth = controller.data(controller.index(0, 0), DockController::ItemWidthRole).toDouble();

    // Increase bar height by 24px
    controller.setBarHeight(initialBarHeight + 24.0);

    // Pill height should increase by 24px
    EXPECT_DOUBLE_EQ(controller.pillHeight(), initialBarHeight + 24.0);
    EXPECT_DOUBLE_EQ(controller.pillHeight() - initialPillHeight, 24.0);

    // Pill width and icon widths MUST remain completely constant
    EXPECT_DOUBLE_EQ(controller.pillWidth(), initialPillWidth);
    EXPECT_DOUBLE_EQ(controller.data(controller.index(0, 0), DockController::ItemWidthRole).toDouble(), initialIconWidth);
}

TEST_F(DockSettingsTest, ChangingIconSizeUpdatesIconDimensions) {
    const double initialIconWidth = controller.data(controller.index(0, 0), DockController::ItemWidthRole).toDouble();

    // Increase icon size from default 48px to 60px
    controller.setIconSize(60.0);

    EXPECT_GT(controller.data(controller.index(0, 0), DockController::ItemWidthRole).toDouble(), initialIconWidth);
    EXPECT_DOUBLE_EQ(controller.iconSize(), 60.0);
}

TEST_F(DockSettingsTest, ChangingItemSpacingUpdatesPillWidth) {
    const double initialWidth = controller.pillWidth();

    // Increase spacing from default 10px to 25px
    controller.setItemSpacing(25.0);

    EXPECT_GT(controller.pillWidth(), initialWidth);
    EXPECT_DOUBLE_EQ(controller.itemSpacing(), 25.0);
}

TEST_F(DockSettingsTest, AutoCornerRadiusMaintainsHalfHeightWhileCustomAllowsSharpEdges) {
    controller.setAutoCornerRadius(true);
    EXPECT_DOUBLE_EQ(controller.pillRadius(), controller.pillHeight() / 2.0);

    // Disable auto pill shape and set a sharp corner radius (e.g. 6px)
    controller.setAutoCornerRadius(false);
    controller.setCornerRadius(6.0);
    EXPECT_DOUBLE_EQ(controller.pillRadius(), 6.0);

    // Reset back to auto pill
    controller.setAutoCornerRadius(true);
    EXPECT_DOUBLE_EQ(controller.pillRadius(), controller.pillHeight() / 2.0);
}

TEST_F(DockSettingsTest, InsertSeparatorPlacesNewDividerAtCorrectIndex) {
    const int initialCount = controller.rowCount();

    // Insert separator after the first item (index 0)
    controller.insertSeparator(0);

    EXPECT_EQ(controller.rowCount(), initialCount + 1);

    auto insertedIdx = controller.index(1, 0);
    EXPECT_EQ(controller.data(insertedIdx, DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
    EXPECT_EQ(controller.data(insertedIdx, DockController::NameRole).toString().toStdString(), "Separator");
}

TEST_F(DockSettingsTest, RemoveSeparatorDeletesDividerAndUpdatesRowCount) {
    // Current separator is at index 2
    ASSERT_EQ(controller.data(controller.index(2, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
    const int countBefore = controller.rowCount();

    controller.removeSeparator(2);

    EXPECT_EQ(controller.rowCount(), countBefore - 1);
    // Index 2 should now be the Downloads stack
    EXPECT_EQ(controller.data(controller.index(2, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::DocumentStack));
}

TEST_F(DockSettingsTest, ChangingIconBottomMarginShiftsIconsWithinPill) {
    const double initialPillY = controller.pillY();
    const double initialPillHeight = controller.pillHeight();
    const double initialIconY = controller.data(controller.index(0, 0), DockController::ItemYRole).toDouble();
    const double delta = 6.0;

    // Increase icon bottom margin by 6px
    controller.setIconBottomMargin(controller.iconBottomMargin() + delta);

    // Pill geometry must remain unchanged
    EXPECT_DOUBLE_EQ(controller.pillY(), initialPillY);
    EXPECT_DOUBLE_EQ(controller.pillHeight(), initialPillHeight);

    // Icon should shift upwards inside the pill by delta
    const double newIconY = controller.data(controller.index(0, 0), DockController::ItemYRole).toDouble();
    EXPECT_NEAR(initialIconY - newIconY, delta, 0.001);
}

TEST_F(DockSettingsTest, ChangingSeparatorSpacingAltersGapsAroundSeparators) {
    const double initialPillWidth = controller.pillWidth();
    const double delta = 8.0;

    // Increase separator spacing by 8px (affects left and right of the separator = +16px total)
    controller.setSeparatorSpacing(controller.separatorSpacing() + delta);

    EXPECT_GT(controller.pillWidth(), initialPillWidth);
    EXPECT_DOUBLE_EQ(controller.pillWidth() - initialPillWidth, delta * 2.0);
}

TEST_F(DockSettingsTest, PillPaddingXHasExtraFivePixelsDefaultCurveClearance) {
    EXPECT_DOUBLE_EQ(controller.pillPaddingX(), 19.0);
}

TEST_F(DockSettingsTest, ChangingMultiInstanceModeUpdatesPropertyAndClampsBounds) {
    EXPECT_EQ(controller.multiInstanceMode(), 0); // Default: MiniWindows (0)

    controller.setMultiInstanceMode(1); // WindowList (1)
    EXPECT_EQ(controller.multiInstanceMode(), 1);

    controller.setMultiInstanceMode(99); // Out-of-bounds clamped to 1
    EXPECT_EQ(controller.multiInstanceMode(), 1);

    controller.setMultiInstanceMode(-5); // Out-of-bounds clamped to 0
    EXPECT_EQ(controller.multiInstanceMode(), 0);
}

TEST_F(DockSettingsTest, ResetSettingsRestoresDefaultValues) {
    controller.setBottomMargin(40.0);
    controller.setBarHeight(80.0);
    controller.setIconBottomMargin(16.0);
    controller.setIconSize(72.0);
    controller.setItemSpacing(30.0);
    controller.setSeparatorSpacing(24.0);
    controller.setAutoCornerRadius(false);
    controller.setCornerRadius(4.0);
    controller.setMultiInstanceMode(1);

    controller.resetSettingsToDefaults();

    EXPECT_DOUBLE_EQ(controller.bottomMargin(), 12.0);
    EXPECT_DOUBLE_EQ(controller.barHeight(), 64.0);
    EXPECT_DOUBLE_EQ(controller.iconBottomMargin(), 8.0);
    EXPECT_DOUBLE_EQ(controller.iconSize(), 48.0);
    EXPECT_DOUBLE_EQ(controller.itemSpacing(), 10.0);
    EXPECT_DOUBLE_EQ(controller.separatorSpacing(), 12.0);
    EXPECT_DOUBLE_EQ(controller.pillPaddingX(), 19.0);
    EXPECT_TRUE(controller.autoCornerRadius());
    EXPECT_EQ(controller.multiInstanceMode(), 0);
}
