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
            .screenName = "",
            .waylandUuid = ""
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
            .screenName = "",
            .waylandUuid = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Row count should increase to 3
    EXPECT_EQ(controller.rowCount(), 3);

    // All pinned apps must be on the left, separator follows, unpinned app is after separator (index 2)
    auto dolphinIdx = controller.index(0, 0);
    EXPECT_EQ(controller.data(dolphinIdx, DockController::TypeRole).toInt(), static_cast<int>(DockItemType::AppLauncher));
    EXPECT_TRUE(controller.data(dolphinIdx, DockController::IsPinnedRole).toBool());

    auto sepIdx = controller.index(1, 0);
    EXPECT_EQ(controller.data(sepIdx, DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));

    auto spotifyIdx = controller.index(2, 0);
    EXPECT_EQ(controller.data(spotifyIdx, DockController::NameRole).toString().toStdString(), "Spotify");
    EXPECT_FALSE(controller.data(spotifyIdx, DockController::IsPinnedRole).toBool());
    EXPECT_TRUE(controller.data(spotifyIdx, DockController::IsRunningRole).toBool());
    EXPECT_TRUE(controller.data(spotifyIdx, DockController::IsFocusedRole).toBool());

    // Close Spotify (windows become empty)
    tracker.simulateWindowChange({});

    // Spotify must be removed from dock, row count back to 2
    EXPECT_EQ(controller.rowCount(), 2);
    auto finalSepIdx = controller.index(1, 0);
    EXPECT_EQ(controller.data(finalSepIdx, DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
}

TEST(DockWindowTrackingTest, UnpinnedAppsArePositionedInTheMiddleBetweenSeparatorAndFolder) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "org.kde.dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "separator_recent", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true},
        DockItem{.id = "/home/cflores/Downloads", .name = "Downloads", .iconName = "folder-download", .type = DockItemType::DocumentStack, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    ASSERT_EQ(controller.rowCount(), 3);

    // Open Spotify and VLC (unpinned)
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 2,
            .appId = "spotify",
            .title = "Spotify",
            .iconName = "spotify",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = false,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = ""
        },
        WindowInstance{
            .id = 3,
            .appId = "vlc",
            .title = "VLC media player",
            .iconName = "vlc",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Row count should be 5: [Dolphin, Separator, Spotify, VLC, Downloads]
    ASSERT_EQ(controller.rowCount(), 5);

    // 0: Dolphin (pinned)
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
    EXPECT_TRUE(controller.data(controller.index(0, 0), DockController::IsPinnedRole).toBool());

    // 1: Separator
    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));

    // 2: Spotify (unpinned, in middle)
    EXPECT_EQ(controller.data(controller.index(2, 0), DockController::NameRole).toString().toStdString(), "Spotify");
    EXPECT_FALSE(controller.data(controller.index(2, 0), DockController::IsPinnedRole).toBool());

    // 3: VLC (unpinned, in middle)
    EXPECT_EQ(controller.data(controller.index(3, 0), DockController::NameRole).toString().toStdString(), "VLC media player");
    EXPECT_FALSE(controller.data(controller.index(3, 0), DockController::IsPinnedRole).toBool());

    // 4: Downloads (folder on far right)
    EXPECT_EQ(controller.data(controller.index(4, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::DocumentStack));
}

TEST(DockWindowTrackingTest, PinningUnpinnedAppMovesItToTheLeftOfSeparator) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "org.kde.dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "separator_recent", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true},
        DockItem{.id = "/home/cflores/Downloads", .name = "Downloads", .iconName = "folder-download", .type = DockItemType::DocumentStack, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    // Open Spotify (unpinned, inserted at index 2)
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 2,
            .appId = "spotify",
            .title = "Spotify",
            .iconName = "spotify",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = ""
        }
    };
    tracker.simulateWindowChange(windows);
    ASSERT_EQ(controller.rowCount(), 4);
    EXPECT_EQ(controller.data(controller.index(2, 0), DockController::NameRole).toString().toStdString(), "Spotify");

    // Pin Spotify (index 2)
    controller.pinApp(2);

    // Row count remains 4, but Spotify is now at index 1 (before separator), and separator is at index 2
    ASSERT_EQ(controller.rowCount(), 4);
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
    EXPECT_TRUE(controller.data(controller.index(0, 0), DockController::IsPinnedRole).toBool());

    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::NameRole).toString().toStdString(), "Spotify");
    EXPECT_TRUE(controller.data(controller.index(1, 0), DockController::IsPinnedRole).toBool());

    EXPECT_EQ(controller.data(controller.index(2, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
    EXPECT_EQ(controller.data(controller.index(3, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::DocumentStack));
}

TEST(DockWindowTrackingTest, UnpinningRunningAppMovesItBetweenSeparatorAndFolder) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "org.kde.dolphin.desktop", .name = "Dolphin", .iconName = "dolphin", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "firefox.desktop", .name = "Firefox", .iconName = "firefox", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "separator_recent", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true},
        DockItem{.id = "/home/cflores/Downloads", .name = "Downloads", .iconName = "folder-download", .type = DockItemType::DocumentStack, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    // Dolphin is running
    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 1,
            .appId = "org.kde.dolphin",
            .title = "Dolphin",
            .iconName = "dolphin",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Dolphin is at index 0 and running
    EXPECT_TRUE(controller.data(controller.index(0, 0), DockController::IsRunningRole).toBool());

    // Unpin Dolphin (index 0)
    controller.unpinApp(0);

    // Row count remains 4: [Firefox (pinned), Separator, Dolphin (unpinned, running), Downloads (folder)]
    ASSERT_EQ(controller.rowCount(), 4);
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::NameRole).toString().toStdString(), "Firefox");
    EXPECT_TRUE(controller.data(controller.index(0, 0), DockController::IsPinnedRole).toBool());

    EXPECT_EQ(controller.data(controller.index(1, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));

    EXPECT_EQ(controller.data(controller.index(2, 0), DockController::NameRole).toString().toStdString(), "Dolphin");
    EXPECT_FALSE(controller.data(controller.index(2, 0), DockController::IsPinnedRole).toBool());
    EXPECT_TRUE(controller.data(controller.index(2, 0), DockController::IsRunningRole).toBool());

    EXPECT_EQ(controller.data(controller.index(3, 0), DockController::TypeRole).toInt(), static_cast<int>(DockItemType::DocumentStack));
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
            .screenName = "",
            .waylandUuid = ""
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
        .screenName = "",
        .waylandUuid = ""
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
            .screenName = "",
            .waylandUuid = ""
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
            .screenName = "",
            .waylandUuid = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Row count should be 2: [Separator (index 0), VS Code (index 1)]
    ASSERT_EQ(controller.rowCount(), 2);
    auto codeIdx = controller.index(1, 0);

    // Should resolve desktop icon ("vscode") from com.microsoft.VSCode.desktop
    EXPECT_EQ(controller.data(codeIdx, DockController::IconNameRole).toString().toStdString(), "vscode");
    EXPECT_FALSE(controller.data(codeIdx, DockController::IsPinnedRole).toBool());

    // Pin app to dock (index 1) - moves VS Code to the left of separator (index 0)
    controller.pinApp(1);
    auto pinnedCodeIdx = controller.index(0, 0);
    EXPECT_TRUE(controller.data(pinnedCodeIdx, DockController::IsPinnedRole).toBool());
    EXPECT_EQ(controller.data(pinnedCodeIdx, DockController::IconNameRole).toString().toStdString(), "vscode");

    auto sepIdx = controller.index(1, 0);
    EXPECT_EQ(controller.data(sepIdx, DockController::TypeRole).toInt(), static_cast<int>(DockItemType::Separator));
}

TEST(DockWindowTrackingTest, MultipleInstancesOfSameApp_AggregatesCountAndReturnsIndividualWindowsList) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "org.kde.konsole.desktop", .name = "Konsole", .iconName = "utilities-terminal", .type = DockItemType::AppLauncher, .isPinned = true},
        DockItem{.id = "separator", .name = "Separator", .iconName = "", .type = DockItemType::Separator, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 101,
            .appId = "org.kde.konsole",
            .title = "cflores@cachyos: ~/projects — Konsole",
            .iconName = "utilities-terminal",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = false,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = "{uuid-101}"
        },
        WindowInstance{
            .id = 102,
            .appId = "org.kde.konsole",
            .title = "htop — Konsole",
            .iconName = "utilities-terminal",
            .isMaximized = false,
            .isMinimized = true,
            .isFullscreen = false,
            .isActive = false,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = "{uuid-102}"
        },
        WindowInstance{
            .id = 103,
            .appId = "org.kde.konsole",
            .title = "nvim — Konsole",
            .iconName = "utilities-terminal",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = "{uuid-103}"
        }
    };
    tracker.simulateWindowChange(windows);

    // Must not duplicate dock items for same app
    EXPECT_EQ(controller.rowCount(), 2);

    auto konsoleIdx = controller.index(0, 0);
    EXPECT_TRUE(controller.data(konsoleIdx, DockController::IsRunningRole).toBool());
    EXPECT_TRUE(controller.data(konsoleIdx, DockController::IsFocusedRole).toBool());
    EXPECT_EQ(controller.data(konsoleIdx, DockController::RunningWindowCountRole).toInt(), 3);

    // Query individual window instances for the switcher popup
    QVariantList winList = controller.getWindowsForItem(0);
    ASSERT_EQ(winList.size(), 3);

    auto win0 = winList[0].toMap();
    auto win1 = winList[1].toMap();
    auto win2 = winList[2].toMap();

    EXPECT_EQ(win0["windowId"].toULongLong(), 101ULL);
    EXPECT_EQ(win0["title"].toString().toStdString(), "cflores@cachyos: ~/projects — Konsole");
    EXPECT_FALSE(win0["isActive"].toBool());

    EXPECT_EQ(win1["windowId"].toULongLong(), 102ULL);
    EXPECT_EQ(win1["title"].toString().toStdString(), "htop — Konsole");
    EXPECT_TRUE(win1["isMinimized"].toBool());

    EXPECT_EQ(win2["windowId"].toULongLong(), 103ULL);
    EXPECT_EQ(win2["title"].toString().toStdString(), "nvim — Konsole");
    EXPECT_TRUE(win2["isActive"].toBool());
    EXPECT_EQ(win2["waylandUuid"].toString().toStdString(), "{uuid-103}");
}

TEST(DockWindowTrackingTest, ActivateAndCloseSpecificWindowInstance_TargetsRequestedWindowId) {
    DockController controller;
    KWinWaylandTracker tracker;

    std::vector<DockItem> items = {
        DockItem{.id = "firefox.desktop", .name = "Firefox", .iconName = "firefox", .type = DockItemType::AppLauncher, .isPinned = true}
    };
    controller.setItems(items);
    controller.setWindowTracker(&tracker);

    std::vector<WindowInstance> windows = {
        WindowInstance{
            .id = 201,
            .appId = "org.mozilla.firefox",
            .title = "GitHub — Mozilla Firefox",
            .iconName = "firefox",
            .isMaximized = false,
            .isMinimized = false,
            .isFullscreen = false,
            .isActive = true,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = ""
        },
        WindowInstance{
            .id = 202,
            .appId = "org.mozilla.firefox",
            .title = "CachyOS Wiki — Mozilla Firefox",
            .iconName = "firefox",
            .isMaximized = false,
            .isMinimized = true,
            .isFullscreen = false,
            .isActive = false,
            .windowCount = 1,
            .screenName = "",
            .waylandUuid = ""
        }
    };
    tracker.simulateWindowChange(windows);

    // Select second instance (id = 202, which was minimized)
    controller.activateWindowInstance(202);

    QVariantList updatedList = controller.getWindowsForItem(0);
    ASSERT_EQ(updatedList.size(), 2);
    EXPECT_FALSE(updatedList[0].toMap()["isActive"].toBool());
    EXPECT_TRUE(updatedList[1].toMap()["isActive"].toBool());
    EXPECT_FALSE(updatedList[1].toMap()["isMinimized"].toBool());

    // Close first instance (id = 201)
    controller.closeWindowInstance(201);

    QVariantList afterCloseList = controller.getWindowsForItem(0);
    ASSERT_EQ(afterCloseList.size(), 1);
    EXPECT_EQ(afterCloseList[0].toMap()["windowId"].toULongLong(), 202ULL);
    EXPECT_EQ(controller.data(controller.index(0, 0), DockController::RunningWindowCountRole).toInt(), 1);
}
