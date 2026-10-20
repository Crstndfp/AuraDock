#pragma once

#include <cmath>

namespace cachy_dock::domain::services {

struct BounceFrame {
    double offsetY{0.0};     // Upward displacement in pixels (positive = upward)
    double scaleX{1.0};      // Squash & stretch X
    double scaleY{1.0};      // Squash & stretch Y
    bool isFinished{false};
};

struct BounceConfig {
    double initialAmplitude{28.0}; // Peak height of first bounce in pixels
    double angularFrequency{14.0}; // Frequency of oscillation (radians/sec)
    double dampingCoeff{2.2};      // Decay rate
    double squashFactor{0.18};     // Magnitude of impact squash deformation
    double stopThreshold{0.8};     // Pixel threshold to terminate
    double maxDurationSec{2.5};    // Fail-safe max duration
};

class BouncePhysicsSolver {
public:
    using Config = BounceConfig;

    BouncePhysicsSolver();
    explicit BouncePhysicsSolver(Config config);

    [[nodiscard]] const Config& getConfig() const noexcept { return m_config; }
    void setConfig(const Config& config) noexcept { m_config = config; }

    [[nodiscard]] BounceFrame evaluate(double timeElapsedSeconds) const noexcept;

private:
    Config m_config;
};

} // namespace cachy_dock::domain::services
