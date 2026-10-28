#include <gtest/gtest.h>
#include "domain/services/SmartAutoHideStateMachine.hpp"

using namespace cachy_dock::domain::services;

TEST(SmartAutoHideStateMachineTest, InitialStateIsVisibleWithFullExclusiveZone) {
    SmartAutoHideStateMachine sm;
    EXPECT_EQ(sm.getState(), AutoHideState::Visible);
    EXPECT_TRUE(sm.shouldBeVisible());
    EXPECT_DOUBLE_EQ(sm.getExclusiveZone(68.0), 68.0);
    EXPECT_DOUBLE_EQ(sm.getTargetVerticalOffset(100.0), 0.0);
}

TEST(SmartAutoHideStateMachineTest, MaximizedWindowAppears_TransitionsToHidingAndZeroExclusiveZone) {
    SmartAutoHideStateMachine sm;

    sm.onMaximizedWindowChanged(true);

    EXPECT_EQ(sm.getState(), AutoHideState::Hiding);
    EXPECT_FALSE(sm.shouldBeVisible());
    EXPECT_DOUBLE_EQ(sm.getExclusiveZone(68.0), 0.0);
    EXPECT_DOUBLE_EQ(sm.getTargetVerticalOffset(100.0), 100.0);

    sm.onHideAnimationFinished();
    EXPECT_EQ(sm.getState(), AutoHideState::HiddenSensor);
    EXPECT_TRUE(sm.isSensorActive());
}

TEST(SmartAutoHideStateMachineTest, PointerEntersSensor_RevealsDock) {
    SmartAutoHideStateMachine sm;
    sm.onMaximizedWindowChanged(true);
    sm.onHideAnimationFinished();
    ASSERT_EQ(sm.getState(), AutoHideState::HiddenSensor);

    sm.onPointerEnteredSensor();
    EXPECT_EQ(sm.getState(), AutoHideState::Revealing);
    EXPECT_TRUE(sm.shouldBeVisible());
    EXPECT_DOUBLE_EQ(sm.getTargetVerticalOffset(100.0), 0.0);

    sm.onRevealAnimationFinished();
    EXPECT_EQ(sm.getState(), AutoHideState::Visible);
}

TEST(SmartAutoHideStateMachineTest, MaximizedWindowCloses_RevealsDockAutomatically) {
    SmartAutoHideStateMachine sm;
    sm.onMaximizedWindowChanged(true);
    sm.onHideAnimationFinished();
    ASSERT_EQ(sm.getState(), AutoHideState::HiddenSensor);

    sm.onMaximizedWindowChanged(false);
    EXPECT_EQ(sm.getState(), AutoHideState::Revealing);
}

TEST(SmartAutoHideStateMachineTest, HoverInDockInhibitsHideEvenIfWindowMaximized) {
    SmartAutoHideStateMachine sm;
    sm.onPointerEnteredDock();
    sm.onMaximizedWindowChanged(true);

    EXPECT_EQ(sm.getState(), AutoHideState::Visible);
    EXPECT_TRUE(sm.shouldBeVisible());

    sm.onPointerLeftDock();
    EXPECT_EQ(sm.getState(), AutoHideState::Hiding);
}

TEST(SmartAutoHideStateMachineTest, FullHeightHalfWidthWindow_FromTopToDockTop_TriggersAutoHide) {
    SmartAutoHideStateMachine sm;
    WindowDodgeParameters params{
        .fallbackScreenWidth = 1920.0,
        .fallbackScreenHeight = 1080.0,
        .dockReservedHeight = 76.0,
        .dockPillLeft = 600.0,
        .dockPillRight = 1320.0
    };

    // Half-width window (x=0, width=960) spanning from below a 32px top bar down to top of dock (1004)
    cachy_dock::domain::entities::WindowInstance halfTiledWin{
        .id = 1,
        .appId = "org.kde.konsole",
        .isMaximized = false,
        .isMinimized = false,
        .isFullscreen = false,
        .isActive = true,
        .x = 0,
        .y = 32,
        .width = 960,
        .height = 972,
        .screenX = 0,
        .screenY = 0,
        .screenWidth = 1920,
        .screenHeight = 1080
    };

    EXPECT_TRUE(SmartAutoHideStateMachine::doesWindowTriggerAutoHide(halfTiledWin, params));

    sm.onWindowsEvaluated({halfTiledWin}, params);
    EXPECT_EQ(sm.getState(), AutoHideState::Hiding);
    EXPECT_FALSE(sm.shouldBeVisible());
    EXPECT_DOUBLE_EQ(sm.getExclusiveZone(76.0), 0.0);
}

TEST(SmartAutoHideStateMachineTest, ResizedTallWindow_OccupyingLargeVerticalSpaceReachingDock_TriggersAutoHide) {
    SmartAutoHideStateMachine sm;
    WindowDodgeParameters params{
        .fallbackScreenWidth = 1920.0,
        .fallbackScreenHeight = 1080.0,
        .dockReservedHeight = 76.0
    };

    // Window not at y=0 (e.g. y=180), but manually resized tall (height=820, bottom=1000)
    cachy_dock::domain::entities::WindowInstance tallResizedWin{
        .id = 2,
        .appId = "firefox",
        .isMaximized = false,
        .isMinimized = false,
        .x = 250,
        .y = 180,
        .width = 700,
        .height = 820,
        .screenWidth = 1920,
        .screenHeight = 1080
    };

    EXPECT_TRUE(SmartAutoHideStateMachine::doesWindowTriggerAutoHide(tallResizedWin, params));
    sm.onWindowsEvaluated({tallResizedWin}, params);
    EXPECT_EQ(sm.getState(), AutoHideState::Hiding);
}

TEST(SmartAutoHideStateMachineTest, SmallFloatingWindows_DoNotTriggerAutoHide) {
    SmartAutoHideStateMachine sm;
    WindowDodgeParameters params{
        .fallbackScreenWidth = 1920.0,
        .fallbackScreenHeight = 1080.0,
        .dockReservedHeight = 76.0
    };

    cachy_dock::domain::entities::WindowInstance smallWin{
        .id = 3,
        .appId = "org.kde.kcalc",
        .isMaximized = false,
        .isMinimized = false,
        .x = 400,
        .y = 250,
        .width = 450,
        .height = 380,
        .screenWidth = 1920,
        .screenHeight = 1080
    };

    EXPECT_FALSE(SmartAutoHideStateMachine::doesWindowTriggerAutoHide(smallWin, params));
    sm.onWindowsEvaluated({smallWin}, params);
    EXPECT_EQ(sm.getState(), AutoHideState::Visible);
    EXPECT_TRUE(sm.shouldBeVisible());
}

TEST(SmartAutoHideStateMachineTest, MultipleWindows_OnlyOneFulfillsTallCondition_TriggersAutoHide) {
    SmartAutoHideStateMachine sm;
    WindowDodgeParameters params{
        .fallbackScreenWidth = 1920.0,
        .fallbackScreenHeight = 1080.0,
        .dockReservedHeight = 76.0
    };

    cachy_dock::domain::entities::WindowInstance smallActive{
        .id = 1, .appId = "kcalc", .isMaximized = false, .isMinimized = false, .isActive = true,
        .x = 500, .y = 300, .width = 300, .height = 350, .screenWidth = 1920, .screenHeight = 1080
    };
    cachy_dock::domain::entities::WindowInstance smallBackground{
        .id = 2, .appId = "dolphin", .isMaximized = false, .isMinimized = false, .isActive = false,
        .x = 100, .y = 120, .width = 600, .height = 420, .screenWidth = 1920, .screenHeight = 1080
    };
    cachy_dock::domain::entities::WindowInstance tallBackground{
        .id = 3, .appId = "code", .isMaximized = false, .isMinimized = false, .isActive = false,
        .x = 1100, .y = 0, .width = 820, .height = 1004, .screenWidth = 1920, .screenHeight = 1080
    };

    EXPECT_TRUE(SmartAutoHideStateMachine::shouldHideForWindows({smallActive, smallBackground, tallBackground}, params));
    sm.onWindowsEvaluated({smallActive, smallBackground, tallBackground}, params);
    EXPECT_EQ(sm.getState(), AutoHideState::Hiding);
}

TEST(SmartAutoHideStateMachineTest, MinimizedTallOrMaximizedWindow_IsIgnoredAndRevealsDock) {
    SmartAutoHideStateMachine sm;
    WindowDodgeParameters params{
        .fallbackScreenWidth = 1920.0,
        .fallbackScreenHeight = 1080.0,
        .dockReservedHeight = 76.0
    };

    cachy_dock::domain::entities::WindowInstance tallWin{
        .id = 1, .appId = "firefox", .isMaximized = true, .isMinimized = false,
        .x = 0, .y = 0, .width = 1920, .height = 1080, .screenWidth = 1920, .screenHeight = 1080
    };

    sm.onWindowsEvaluated({tallWin}, params);
    sm.onHideAnimationFinished();
    ASSERT_EQ(sm.getState(), AutoHideState::HiddenSensor);

    // Minimize the window: dock must reveal itself
    tallWin.isMinimized = true;
    EXPECT_FALSE(SmartAutoHideStateMachine::doesWindowTriggerAutoHide(tallWin, params));
    sm.onWindowsEvaluated({tallWin}, params);
    EXPECT_EQ(sm.getState(), AutoHideState::Revealing);
    EXPECT_TRUE(sm.shouldBeVisible());
}
