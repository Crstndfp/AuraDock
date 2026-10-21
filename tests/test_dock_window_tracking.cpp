#include <gtest/gtest.h>
#include "application/DockController.hpp"
#include "infrastructure/trackers/KWinWaylandTracker.hpp"
#include "domain/entities/DockItem.hpp"
#include "domain/entities/WindowInstance.hpp"

using namespace cachy_dock::application;
using namespace cachy_dock::infrastructure::trackers;
using namespace cachy_dock::domain::entities;

TEST(DockWindowTrackingTest, RunningPinnedAppShowsRunningStateAndFocusIndicator) {
    DockController controller;
    KWinWaylandTracker tracker;

    // Create 2 pinned apps + separator
    std::vector<DockItem> items = {
        DockItem{.id = "org.kde.dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "firefox.desktop", .name = "Firefox", .iconName = "firefox", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "separator", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    // Initial state: not running
    auto dolphinIdx = controller.index(0, 0);
    EXPECT_FALSE(controller.data(dolphinIdx, DockController::IsRunningRole).toBool());
    EXPECT_FALSE(controller.data(dolphinIdx, DockController::IsFocusedRole).toBool());

    // Simulate Dolphin window opening and becoming active
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 1,
            .appId = "org.kde.dolphin",
            .title = "Dolphin - File Manager",
            .iconName = "dolphin",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Verify Dolphin is running and focused
    EXPECT_TRUE(controller.data(dolphinIdx, DockController::IsRunningRole).toBool());
    EXPECT_TRUE(controller.data(dolphinIdx, DockController::IsFocusedRole).toBool());
    EXPECT_EQ(controller.data(dolphinIdx, DockController::RunningWindowCountRole).toInt(), 1);

    // Firefox is not running
    auto firefoxIdx = controller.index(1, 0);
    EXPECT_FALSE(controller.data(firefoxIdx, DockController::IsRunningRole).toBool());
}

TEST(DockWindowTrackingTest, UnpinnedRunningAppIsDynamicallyAddedAndRemovedOnClose) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "org.kde.dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "separator", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    ASSERT_EQ(controller.rowCount(), 2);

    // Open an unpinned app (e.g. Spotify)
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 2,
            .appId = "spotify",
            .title = "Spotify - Music for everyone",
            .iconName = "spotify",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Row count should increase to 3
    EXPECT_EQ(controller.rowCount(), 3);

    // The unpinned app should be inserted right before the separator (index 1)
    auto spotifyIdx = controller.index(1, 0);
    EXPECT_EQ(controller.data(spotifyIdx, DockController::NameRole).toString().toStdString(), "Spotify");
    EXPECT_FALSE(controller.data(spotifyIdx, DockController::IsPinnedRole).toBool());
    EXPECT_TRUE(controller.data(spotifyIdx, DockController::IsRunningRole).toBool());
    EXPECT_TRUE(controller.data(spotifyIdx, DockController::IsFocusedRole).toBool());

    // Close Spotify (windows become empty)
    tracker.simulateWindowChange({});

    // Spotify must be removed from dock, row count back to 2
    EXPECT_EQ(controller.rowCount(), 2);
    auto sepIdx = controller.index(1, 0);
    EXPECT_EQ(controller.data(sepIdx, DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
}

TEST(DockWindowTrackingTest, DisambiguatesPrefixAppsSuchAsAntigravityAndAntigravityIde) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "antigravity.desktop", .name = "Antigravity", .iconName = "antigravity", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "antigravity-ide.desktop", .name = "Antigravity IDE", .iconName = "antigravity-ide", .type = DockItemType::AppLauncher, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    auto agIdx = controller.index(0, 0);
    auto ideIdx = controller.index(1, 0);

    // Only Antigravity is open
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 10,
            .appId = "antigravity",
            .title = "Antigravity",
            .iconName = "antigravity",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = ""
        }
    };
    tracker.simulateWindowChange(windows);

    EXPECT_TRUE(controller.data(agIdx, DockController::IsRunningRole).toBool());
    EXPECT_FALSE(controller.data(ideIdx, DockController::IsRunningRole).toBool());

    // Both are open
    windows.push_back(WindowInstance{
        .id = 11,
        .appId = "antigravity-ide",
        .title = "Antigravity IDE",
        .iconName = "antigravity-ide",
        .isMaximized = false,
        .isMinimized = false,
        .isFullscreen = false,
        .isActive = false,
        .windowCount = 1,
        .screenName = ""
    });
    tracker.simulateWindowChange(windows);

    EXPECT_TRUE(controller.data(agIdx, DockController::IsRunningRole).toBool());
    EXPECT_TRUE(controller.data(ideIdx, DockController::IsRunningRole).toBool());
}

TEST(DockWindowTrackingTest, ReverseDnsMatchingWorksForFirefox) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "/usr/share/applications/firefox.desktop", .name = "Firefox", .iconName = "firefox", .type = DockItemType::AppLauncher, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    auto ffIdx = controller.index(0, 0);
    EXPECT_FALSE(controller.data(ffIdx, DockController::IsRunningRole).toBool());

    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 12,
            .appId = "org.mozilla.firefox",
            .title = "Mozilla Firefox",
            .iconName = "firefox",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = ""
        }
    };
    tracker.simulateWindowChange(windows);

    EXPECT_TRUE(controller.data(ffIdx, DockController::IsRunningRole).toBool());
    EXPECT_TRUE(controller.data(ffIdx, DockController::IsFocusedRole).toBool());
}

TEST(DockWindowTrackingTest, UnpinnedReverseDnsAppResolvesDesktopIconAndCanBePinned) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "separator", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    ASSERT_EQ(controller.rowCount(), 1);

    // Simulate VS Code window appearing
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 20,
            .appId = "com.microsoft.VSCode",
            .title = "Visual Studio Code",
            .iconName = "com.microsoft.VSCode",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Row count should be 2 (VS Code inserted before separator)
    ASSERT_EQ(controller.rowCount(), 2);
    auto codeIdx = controller.index(0, 0);

    // Should resolve desktop icon ("vscode") from com.microsoft.VSCode.desktop
    EXPECT_EQ(controller.data(codeIdx, DockController::IconNameRole).toString().toStdString(), "vscode");
    EXPECT_FALSE(controller.data(codeIdx, DockController::IsPinnedRole).toBool());

    // Pin app to dock
    controller.pinApp(0);
    EXPECT_TRUE(controller.data(codeIdx, DockController::IsPinnedRole).toBool());
    EXPECT_EQ(controller.data(codeIdx, DockController::IconNameRole).toString().toStdString(), "vscode");
}
