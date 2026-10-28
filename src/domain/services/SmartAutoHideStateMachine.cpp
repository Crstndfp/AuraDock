#include "domain/services/SmartAutoHideStateMachine.hpp"
#include <algorithm>
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

bool SmartAutoHideStateMachine::doesWindowTriggerAutoHide(
    const entities::WindowInstance& win,
    const WindowDodgeParameters& params) noexcept
{
    // 1. Minimized windows are not visible on screen and must never hide the dock
    if (win.isMinimized) {
        return false;
    }

    // 2. Ignore the dock's own preferences/settings dialog so user can see live previews
    if (win.appId == "cachy-dock" || win.appId == "org.cachyos.dock") {
        return false;
    }

    // 3. Maximized or Fullscreen windows always trigger auto-hide
    if (win.isMaximized || win.isFullscreen) {
        return true;
    }

    // 4. Geometry-based evaluation for full-height, tall, or dock-overlapping windows
    if (win.width <= 0 || win.height <= 0) {
        return false;
    }

    const bool hasExplicitScreen = (win.screenWidth > 0 && win.screenHeight > 0);
    const double scrX = hasExplicitScreen ? static_cast<double>(win.screenX) : 0.0;
    const double scrY = hasExplicitScreen ? static_cast<double>(win.screenY) : 0.0;
    const double scrH = hasExplicitScreen
        ? static_cast<double>(win.screenHeight)
        : (params.fallbackScreenHeight > 0.0 ? params.fallbackScreenHeight : 1080.0);

    const double dockReserved = std::max(48.0, params.dockReservedHeight);
    const double screenTop = scrY;
    const double screenBottom = scrY + scrH;
    const double dockTopY = screenBottom - dockReserved;
    const double usableHeight = std::max(200.0, dockTopY - screenTop);

    const double winTop = static_cast<double>(win.y);
    const double winHeight = static_cast<double>(win.height);
    const double winBottom = winTop + winHeight;

    // Allowance for top desktop panels (e.g. KDE Plasma panel / Waybar) and titlebars
    const double topAllowance = std::max(76.0, scrH * 0.08);
    // Tolerance near the top edge of the dock for tiling gaps / window shadows
    const double bottomTolerance = 28.0;

    // Case A: Window spans from the top of the workspace down to the top of the dock
    // (Handles full-height windows regardless of width: half-tiled, narrow terminals, sidebars)
    if (winTop <= (screenTop + topAllowance) && winBottom >= (dockTopY - bottomTolerance)) {
        return true;
    }

    // Case B: Window height was resized to occupy significant vertical space (>= 70% of usable height)
    // and its bottom edge reaches or crosses the top of the dock
    if (winHeight >= (usableHeight * 0.70) && winBottom >= (dockTopY - bottomTolerance)) {
        return true;
    }

    // Case C: Window occupies almost the entire usable vertical height (>= 82%) near the dock
    if (winHeight >= (usableHeight * 0.82) && winBottom >= (dockTopY - (bottomTolerance * 2.0))) {
        return true;
    }

    // Case D: Moderately tall window intruding directly into the dock pill zone
    if (winHeight >= (usableHeight * 0.45) && winBottom > (dockTopY + 8.0)) {
        const double winLeft = static_cast<double>(win.x) - scrX;
        const double winRight = winLeft + static_cast<double>(win.width);
        if (params.dockPillRight <= params.dockPillLeft ||
            (winRight >= params.dockPillLeft && winLeft <= params.dockPillRight)) {
            return true;
        }
    }

    return false;
}

bool SmartAutoHideStateMachine::shouldHideForWindows(
    const std::vector<entities::WindowInstance>& windows,
    const WindowDodgeParameters& params) noexcept
{
    return std::any_of(windows.begin(), windows.end(), [&params](const entities::WindowInstance& win) {
        return doesWindowTriggerAutoHide(win, params);
    });
}

void SmartAutoHideStateMachine::onMaximizedWindowChanged(bool hasMaximizedOnScreen) {
    m_hasMaximizedOnScreen = hasMaximizedOnScreen;
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onWindowsEvaluated(
    const std::vector<entities::WindowInstance>& windows,
    const WindowDodgeParameters& params)
{
    m_hasMaximizedOnScreen = shouldHideForWindows(windows, params);
    evaluateDesiredVisibility();
}

void SmartAutoHideStateMachine::onPopupOrDialogActiveChanged(bool isActive) {
    m_popupOrDialogActive = isActive;
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

    // Pointer inside or active popup/settings dialog keeps dock visible
    if (m_pointerInsideDock || m_pointerInsideSensor || m_popupOrDialogActive) {
        return true;
    }

    if (m_config.mode == AutoHideMode::SmartMaximizedOnly) {
        // Hide if any window on screen triggers smart auto-hide
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
