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
