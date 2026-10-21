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
        controller.setItems(items);
    }

    DockController controller;
};

TEST_F(DockSettingsTest, ChangingBottomMarginUpdatesPillY) {
    const double initialPillY = controller.pillY();
    
    // Increase bottom margin by 20px
    controller.setBottomMargin(controller.bottomMargin() + 20.0);

    // Dock pill should move upwards (lower Y value)
    EXPECT_LT(controller.pillY(), initialPillY);
}

TEST_F(DockSettingsTest, ChangingIconSizeUpdatesPillHeightAndDimensions) {
    const double initialHeight = controller.pillHeight();

    // Increase icon size from default 48px to 64px
    controller.setIconSize(64.0);

    EXPECT_GT(controller.pillHeight(), initialHeight);
    EXPECT_DOUBLE_EQ(controller.iconSize(), 64.0);
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

TEST_F(DockSettingsTest, ResetSettingsRestoresDefaultValues) {
    controller.setBottomMargin(40.0);
    controller.setIconSize(72.0);
    controller.setItemSpacing(30.0);
    controller.setAutoCornerRadius(false);
    controller.setCornerRadius(4.0);

    controller.resetSettingsToDefaults();

    EXPECT_DOUBLE_EQ(controller.bottomMargin(), 12.0);
    EXPECT_DOUBLE_EQ(controller.iconSize(), 48.0);
    EXPECT_DOUBLE_EQ(controller.itemSpacing(), 10.0);
    EXPECT_TRUE(controller.autoCornerRadius());
}
