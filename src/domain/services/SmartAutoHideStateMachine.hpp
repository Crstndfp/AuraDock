#pragma once

#include "domain/entities/WindowInstance.hpp"
#include <vector>

namespace cachy_dock::domain::services {

enum class AutoHideState {
    Visible,
    Hiding,
    HiddenSensor,
    Revealing
};

enum class AutoHideMode {
    SmartMaximizedOnly,  // Hide when any window is maximized, fullscreen, or tall enough to reach/overlap the dock
    AlwaysAutoHide,      // Always hide unless pointer hovers edge
    Disabled             // Never hide
};

struct AutoHideConfig {
    AutoHideMode mode{AutoHideMode::SmartMaximizedOnly};
    double dwellTimeThresholdMs{100.0}; // Dwell time in sensor strip before revealing
    double slideDurationMs{220.0};
};

struct WindowDodgeParameters {
    double fallbackScreenWidth{1920.0};
    double fallbackScreenHeight{1080.0};
    double dockReservedHeight{76.0};
    double dockPillLeft{0.0};
    double dockPillRight{0.0};
};

class SmartAutoHideStateMachine {
public:
    using Config = AutoHideConfig;

    SmartAutoHideStateMachine();
    explicit SmartAutoHideStateMachine(Config config);

    [[nodiscard]] AutoHideState getState() const noexcept { return m_state; }
    [[nodiscard]] const Config& getConfig() const noexcept { return m_config; }
    void setConfig(const Config& config) noexcept;

    // Geometry & state evaluation for one or multiple windows
    [[nodiscard]] static bool doesWindowTriggerAutoHide(
        const entities::WindowInstance& win,
        const WindowDodgeParameters& params = WindowDodgeParameters{}) noexcept;

    [[nodiscard]] static bool shouldHideForWindows(
        const std::vector<entities::WindowInstance>& windows,
        const WindowDodgeParameters& params = WindowDodgeParameters{}) noexcept;

    // External event inputs
    void onMaximizedWindowChanged(bool hasMaximizedOnScreen);
    void onWindowsEvaluated(const std::vector<entities::WindowInstance>& windows,
                            const WindowDodgeParameters& params = WindowDodgeParameters{});
    void onPopupOrDialogActiveChanged(bool isActive);
    void onPointerEnteredDock();
    void onPointerLeftDock();
    void onPointerEnteredSensor();
    void onPointerLeftSensor();
    void onHideAnimationFinished();
    void onRevealAnimationFinished();

    // Query states for presentation & Wayland layer-shell
    [[nodiscard]] bool shouldBeVisible() const noexcept;
    [[nodiscard]] double getExclusiveZone(double dockBaseHeight) const noexcept;
    [[nodiscard]] double getTargetVerticalOffset(double slideDistance) const noexcept;
    [[nodiscard]] bool isSensorActive() const noexcept;

private:
    void evaluateDesiredVisibility();

    Config m_config;
    AutoHideState m_state{AutoHideState::Visible};
    bool m_hasMaximizedOnScreen{false};
    bool m_pointerInsideDock{false};
    bool m_pointerInsideSensor{false};
    bool m_popupOrDialogActive{false};
};

} // namespace cachy_dock::domain::services
