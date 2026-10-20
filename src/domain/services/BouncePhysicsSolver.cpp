#include "domain/services/BouncePhysicsSolver.hpp"
#include <algorithm>
#include <utility>

namespace cachy_dock::domain::services {

BouncePhysicsSolver::BouncePhysicsSolver()
    : m_config(BounceConfig{}) {}

BouncePhysicsSolver::BouncePhysicsSolver(Config config)
    : m_config(std::move(config)) {}

BounceFrame BouncePhysicsSolver::evaluate(double timeElapsedSeconds) const noexcept {
    if (timeElapsedSeconds < 0.0) {
        return BounceFrame{.offsetY = 0.0, .scaleX = 1.0, .scaleY = 1.0, .isFinished = false};
    }

    if (timeElapsedSeconds >= m_config.maxDurationSec) {
        return BounceFrame{.offsetY = 0.0, .scaleX = 1.0, .scaleY = 1.0, .isFinished = true};
    }

    const double envelope = m_config.initialAmplitude * std::exp(-m_config.dampingCoeff * timeElapsedSeconds);

    if (envelope < m_config.stopThreshold && timeElapsedSeconds > 0.5) {
        return BounceFrame{.offsetY = 0.0, .scaleX = 1.0, .scaleY = 1.0, .isFinished = true};
    }

    const double sinPhase = std::abs(std::sin(m_config.angularFrequency * timeElapsedSeconds));
    const double offsetY = envelope * sinPhase;

    const double cosPhase = std::cos(m_config.angularFrequency * timeElapsedSeconds);
    const double groundProximity = 1.0 - std::min(1.0, offsetY / (m_config.initialAmplitude * 0.4 + 0.001));

    double deformation = groundProximity * m_config.squashFactor * std::exp(-m_config.dampingCoeff * 0.7 * timeElapsedSeconds);
    if (offsetY > 4.0) {
        deformation = -0.08 * std::abs(cosPhase) * std::exp(-m_config.dampingCoeff * timeElapsedSeconds);
    }

    const double scaleX = 1.0 + deformation;
    const double scaleY = 1.0 - deformation;

    return BounceFrame{
        .offsetY = offsetY,
        .scaleX = std::clamp(scaleX, 0.7, 1.4),
        .scaleY = std::clamp(scaleY, 0.7, 1.4),
        .isFinished = false
    };
}

} // namespace cachy_dock::domain::services
