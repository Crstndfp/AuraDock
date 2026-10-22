#include <gtest/gtest.h>
#include <QFile>
#include "application/DockController.hpp"
#include "domain/entities/DockItem.hpp"

using namespace cachy_dock::application;
using namespace cachy_dock::domain::entities;

class DockSettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        DockController::setConfigPathForTesting(QStringLiteral("/tmp/test_cachy_dock.conf"));
        std::vector<DockItem> items = {
            DockItem{.id = "dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
            DockItem{.id = "firefox.desktop", .name = "Firefox", .iconName = "firefox", .type = DockItemType::AppLauncher, .isPinned = true},
            DockItem{.id = "separator", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true},
            DockItem{.id = "downloads", .name = "Downloads", .iconName = "folder-download", .type = DockItemType::DocumentStack, .isPinned = true}
        };
        controller.resetSettingsToDefaults();
        controller.setItems(items);
    }

    void TearDown() override {
        DockController::setConfigPathForTesting(QString());
        QFile::remove(QStringLiteral("/tmp/test_cachy_dock.conf"));
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

TEST_F(DockSettingsTest, SaveSettingsAndLoadSettings_PersistsAllAppearanceValuesAndItems) {
    // Customize appearance
    controller.setBottomMargin(28.0);
    controller.setBarHeight(76.0);
    controller.setIconBottomMargin(14.0);
    controller.setIconSize(56.0);
    controller.setItemSpacing(16.0);
    controller.setSeparatorSpacing(18.0);
    controller.setPillPaddingX(24.0);
    controller.setAutoCornerRadius(false);
    controller.setCornerRadius(10.0);
    controller.setMultiInstanceMode(1);

    // Customize items
    std::vector<DockItem> customItems = {
        DockItem{.id = "custom_app.desktop", .name = "Custom App", .iconName = "custom_icon", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "sep_custom", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true},
        DockItem{.id = "/home/cflores/Documents", .name = "Documents", .iconName = "folder", .type = DockItemType::DocumentStack, .isPinned = true}
    };
    controller.setItems(customItems);

    EXPECT_TRUE(controller.hasSavedItems());
    EXPECT_EQ(controller.rowCount(), 3);

    // Create a new controller instance which automatically executes loadSettings() in its constructor
    DockController freshController;
    EXPECT_TRUE(freshController.hasSavedItems());
    EXPECT_EQ(freshController.rowCount(), 3);

    // Verify appearance persistence
    EXPECT_DOUBLE_EQ(freshController.bottomMargin(), 28.0);
    EXPECT_DOUBLE_EQ(freshController.barHeight(), 76.0);
    EXPECT_DOUBLE_EQ(freshController.iconBottomMargin(), 14.0);
    EXPECT_DOUBLE_EQ(freshController.iconSize(), 56.0);
    EXPECT_DOUBLE_EQ(freshController.itemSpacing(), 16.0);
    EXPECT_DOUBLE_EQ(freshController.separatorSpacing(), 18.0);
    EXPECT_DOUBLE_EQ(freshController.pillPaddingX(), 24.0);
    EXPECT_FALSE(freshController.autoCornerRadius());
    EXPECT_DOUBLE_EQ(freshController.cornerRadius(), 10.0);
    EXPECT_EQ(freshController.multiInstanceMode(), 1);

    // Verify items persistence
    EXPECT_EQ(freshController.data(freshController.index(0, 0), DockController::NameRole).toString().toStdString(), "Custom App");
    EXPECT_EQ(freshController.data(freshController.index(0, 0), DockController::IconNameRole).toString().toStdString(), "custom_icon");
    EXPECT_EQ(freshController.data(freshController.index(1, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
    EXPECT_EQ(freshController.data(freshController.index(2, 0), DockController::NameRole).toString().toStdString(), "Documents");
    EXPECT_EQ(freshController.data(freshController.index(2, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::DocumentStack));
}

TEST_F(DockSettingsTest, MaskUpdateRequired_EmitsDynamicCornerRadiusMatchingSettings) {
    double capturedRadius = -1.0;
    QRectF capturedPillRect;

    QObject::connect(&controller, &DockController::maskUpdateRequired,
                     [&capturedRadius, &capturedPillRect](const QRectF &pillRect, double radius, const std::vector<QRectF> &activeIconRects) {
        Q_UNUSED(activeIconRects);
        capturedPillRect = pillRect;
        capturedRadius = radius;
    });

    // Disable autoCornerRadius and set custom corner radius to 8.0px
    controller.setAutoCornerRadius(false);
    controller.setCornerRadius(8.0);

    EXPECT_DOUBLE_EQ(capturedRadius, 8.0);
    EXPECT_DOUBLE_EQ(controller.pillRadius(), 8.0);

    // Change to 0.0px (sharp square corners)
    controller.setCornerRadius(0.0);
    EXPECT_DOUBLE_EQ(capturedRadius, 0.0);
    EXPECT_DOUBLE_EQ(controller.pillRadius(), 0.0);

    // Re-enable autoCornerRadius: corner radius must restore to pillHeight / 2.0 (32.0)
    controller.setAutoCornerRadius(true);
    EXPECT_DOUBLE_EQ(capturedRadius, controller.pillHeight() / 2.0);
    EXPECT_DOUBLE_EQ(controller.pillRadius(), controller.pillHeight() / 2.0);
}

TEST_F(DockSettingsTest, MovePinnedItem_MovesItemRightAndPersistsOrder) {
    // Initial order: [0]=Dolphin, [1]=Firefox, [2]=Separator, [3]=Downloads
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::NameRole).toString().toStdString(), "Firefox");

    EXPECT_TRUE(controller.canMoveItemRight(0));
    EXPECT_FALSE(controller.canMoveItemLeft(0));
    EXPECT_FALSE(controller.canMoveItemRight(1)); // 1 is last pinned item before separator at 2
    EXPECT_TRUE(controller.canMoveItemLeft(1));

    // Move Dolphin (0) to position 1
    EXPECT_TRUE(controller.movePinnedItem(0, 1));

    // New order: [0]=Firefox, [1]=Dolphin, [2]=Separator, [3]=Downloads
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Firefox");
    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
    EXPECT_EQ(controller.data(controller.index(2, 0), DockController::NameRole).toString().toStdString(), "Separator");
    EXPECT_EQ(controller.data(controller.index(3, 0), DockController::NameRole).toString().toStdString(), "Downloads");

    // Verify order was persisted to test config
    DockController freshController;
    freshController.loadSettings();
    EXPECT_EQ(freshController.data(freshController.index(0, 0), DockController::NameRole).toString().toStdString(), "Firefox");
    EXPECT_EQ(freshController.data(freshController.index(1, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
}

TEST_F(DockSettingsTest, MovePinnedItem_EnforcesBoundaryLimitsAndRejectsSeparatorOrUnpinnedMove) {
    // Attempting to move past separator (index 2) must be rejected
    EXPECT_FALSE(controller.movePinnedItem(0, 2));
    EXPECT_FALSE(controller.movePinnedItem(0, 3));
    EXPECT_FALSE(controller.movePinnedItem(0, -1));

    // Attempting to move the separator (index 2) or folder (index 3) must be rejected
    EXPECT_FALSE(controller.movePinnedItem(2, 0));
    EXPECT_FALSE(controller.movePinnedItem(3, 0));

    // Moving to same index is a valid no-op
    EXPECT_TRUE(controller.movePinnedItem(0, 0));

    // moveItemRight / moveItemLeft helpers
    EXPECT_TRUE(controller.moveItemRight(0)); // Dolphin (0) moves to 1
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Firefox");
    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::NameRole).toString().toStdString(), "Dolphin");

    EXPECT_TRUE(controller.moveItemLeft(1)); // Dolphin (1) moves back to 0
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::NameRole).toString().toStdString(), "Firefox");
}

TEST_F(DockSettingsTest, ChangingDockOpacityAndTransparencyUpdatesValuesAndPersists) {
    // Default opacity is 0.42 (macOS frosted glass), transparency is 0.58
    EXPECT_NEAR(controller.dockOpacity(), 0.42, 0.001);
    EXPECT_NEAR(controller.dockTransparency(), 0.58, 0.001);

    // Setting opacity updates transparency
    controller.setDockOpacity(0.80);
    EXPECT_NEAR(controller.dockOpacity(), 0.80, 0.001);
    EXPECT_NEAR(controller.dockTransparency(), 0.20, 0.001);

    // Setting transparency updates opacity
    controller.setDockTransparency(0.75);
    EXPECT_NEAR(controller.dockTransparency(), 0.75, 0.001);
    EXPECT_NEAR(controller.dockOpacity(), 0.25, 0.001);

    // Clamping limits
    controller.setDockOpacity(1.5);
    EXPECT_DOUBLE_EQ(controller.dockOpacity(), 1.0);
    EXPECT_DOUBLE_EQ(controller.dockTransparency(), 0.0);

    controller.setDockOpacity(-0.5);
    EXPECT_DOUBLE_EQ(controller.dockOpacity(), 0.0);
    EXPECT_DOUBLE_EQ(controller.dockTransparency(), 1.0);

    // Persist to test configuration
    controller.setDockOpacity(0.35);
    DockController freshController;
    freshController.loadSettings();
    EXPECT_NEAR(freshController.dockOpacity(), 0.35, 0.001);
    EXPECT_NEAR(freshController.dockTransparency(), 0.65, 0.001);

    // Reset to defaults
    controller.resetSettingsToDefaults();
    EXPECT_NEAR(controller.dockOpacity(), 0.42, 0.001);
    EXPECT_NEAR(controller.dockTransparency(), 0.58, 0.001);
}
