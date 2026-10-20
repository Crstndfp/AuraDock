#include "domain/services/SmartAutoHideStateMachine.hpp"
#include <utility>

namespace cachy_dock::domain::services {

SmartAutoHideStateMachine::SmartAutoHideStateMachine()
    : m_config(AutoHideConfig{}) {}

SmartAutoHideStateMachine::SmartAutoHideStateMachine(Config config)
    : m_config(std::move(config)) {}

void SmartAutoHideStateMachine::setConfig(const Config& config) noexcept {
    m_config = config;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onMaximizedWindowChanged(bool hasMaximizedOnScreen) {
    m_hasMaximizedOnScreen = hasMaximizedOnScreen;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onPointerEnteredDock() {
    m_pointerInsideDock = true;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onPointerLeftDock() {
    m_pointerInsideDock = false;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onPointerEnteredSensor() {
    m_pointerInsideSensor = true;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onPointerLeftSensor() {
    m_pointerInsideSensor = false;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onHideAnimationFinished() {
    if (m_state == AutoHideState::Hiding) {
        m_state = AutoHideState::HiddenSensor;
        // Re-evaluate in case pointer entered sensor while hiding
        evaluateDesiredVisibility();
    }
}

void SmartAutoHideStateMachine::onRevealAnimationFinished() {
    if (m_state == AutoHideState::Revealing) {
        m_state = AutoHideState::Visible;
        evaluateDesiredVisibility();
    }
}

bool SmartAutoHideStateMachine::shouldBeVisible() const noexcept {
    if (m_config.mode == AutoHideMode::Disabled) {
        return true;
    }

    // Pointer inside keeps dock visible
    if (m_pointerInsideDock || m_pointerInsideSensor) {
        return true;
    }

    if (m_config.mode == AutoHideMode::SmartMaximizedOnly) {
        // Only hide if a window is maximized on screen
        return !m_hasMaximizedOnScreen;
    }

    if (m_config.mode == AutoHideMode::AlwaysAutoHide) {
        return false;
    }

    return true;
}

void SmartAutoHideStateMachine::evaluateDesiredVisibility() {
    const bool desireVisible = shouldBeVisible();

    if (desireVisible) {
        if (m_state == AutoHideState::HiddenSensor || m_state == AutoHideState::Hiding) {
            m_state = AutoHideState::Revealing;
        }
    } else {
        if (m_state == AutoHideState::Visible || m_state == AutoHideState::Revealing) {
            m_state = AutoHideState::Hiding;
        }
    }
}

double SmartAutoHideStateMachine::getExclusiveZone(double dockBaseHeight) const noexcept {
    // Only reserve exclusive desktop area when docked and visible without auto-hide active
    if (m_state == AutoHideState::Visible && !m_hasMaximizedOnScreen && m_config.mode != AutoHideMode::AlwaysAutoHide) {
        return dockBaseHeight;
    }
    return 0.0;
}

double SmartAutoHideStateMachine::getTargetVerticalOffset(double slideDistance) const noexcept {
    switch (m_state) {
        case AutoHideState::Visible:
        case AutoHideState::Revealing:
            return 0.0;
        case AutoHideState::Hiding:
        case AutoHideState::HiddenSensor:
            return slideDistance;
    }
    return 0.0;
}

bool SmartAutoHideStateMachine::isSensorActive() const noexcept {
    return m_state == AutoHideState::HiddenSensor;
}

} // namespace cachy_dock::domain::services
