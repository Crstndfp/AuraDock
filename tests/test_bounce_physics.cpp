#include <gtest/gtest.h>
#include "domain/services/BouncePhysicsSolver.hpp"

using namespace cachy_dock::domain::services;

TEST(BouncePhysicsSolverTest, InitialStateAtTimeZero_StartsAtGround) {
    BouncePhysicsSolver solver;
    auto frame = solver.evaluate(0.0);

    EXPECT_DOUBLE_EQ(frame.offsetY, 0.0);
    EXPECT_FALSE(frame.isFinished);
}

TEST(BouncePhysicsSolverTest, OscillationReachesPeakAndDecays) {
    BouncePhysicsSolver solver;

    // Peak of first bounce occurs near pi / (2 * omega)
    // omega = 14.0 rad/s -> t_peak approx 3.14159 / 28.0 approx 0.112s
    auto peakFrame = solver.evaluate(0.112);
    EXPECT_GT(peakFrame.offsetY, 15.0);

    // Later bounce peak (e.g. at t approx 0.56s) must be lower due to exponential damping
    auto laterPeakFrame = solver.evaluate(0.56);
    EXPECT_LT(laterPeakFrame.offsetY, peakFrame.offsetY);
}

TEST(BouncePhysicsSolverTest, TerminatesAfterThresholdOrMaxDuration) {
    BouncePhysicsSolver solver;

    auto finishedFrame = solver.evaluate(2.6);
    EXPECT_TRUE(finishedFrame.isFinished);
    EXPECT_DOUBLE_EQ(finishedFrame.offsetY, 0.0);
}

TEST(BouncePhysicsSolverTest, AppliesSquashAndStretch) {
    BouncePhysicsSolver solver;

    // Evaluate across a sample trajectory
    bool sawSquash = false;
    for (double t = 0.0; t < 1.0; t += 0.02) {
        auto frame = solver.evaluate(t);
        if (frame.scaleX > 1.02 && frame.scaleY < 0.98) {
            sawSquash = true;
            break;
        }
    }
    EXPECT_TRUE(sawSquash);
}
