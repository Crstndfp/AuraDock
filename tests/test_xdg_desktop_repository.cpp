#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QStandardPaths>
#include "infrastructure/xdg/XdgDesktopRepository.hpp"
#include "infrastructure/xdg/ThemeIconProvider.hpp"

using namespace cachy_dock::infrastructure::xdg;

TEST(XdgDesktopRepositoryTest, DefaultPinnedAppsListReturnsItemsWithDownloadsStack) {
    XdgDesktopRepository repo;
    auto items = repo.getDefaultPinnedApps();

    EXPECT_FALSE(items.empty());

    // Should include separator and document stack
    bool foundSeparator = false;
    bool foundDownloads = false;

    for (const auto &item : items) {
        if (item.type == cachy_dock::domain::entities::DockItemType::Separator) {
            foundSeparator = true;
        }
        if (item.type == cachy_dock::domain::entities::DockItemType::DocumentStack) {
            foundDownloads = true;
        }
    }

    EXPECT_TRUE(foundSeparator);
    EXPECT_TRUE(foundDownloads);
}

TEST(XdgDesktopRepositoryTest, ResolveNonExistentDesktopFile_ReturnsNullopt) {
    XdgDesktopRepository repo;
    auto item = repo.resolveDesktopFile("definitely_non_existent_app_xyz_123.desktop");
    EXPECT_FALSE(item.has_value());
}

TEST(XdgDesktopRepositoryTest, LaunchDesktopIdWithNonExistentReturnsFalse) {
    XdgDesktopRepository repo;
    EXPECT_FALSE(repo.launchDesktopId("/non/existent/path/app.desktop", false));
}

TEST(XdgDesktopRepositoryTest, ResolveReverseDnsWildcard_ResolvesDolphinDesktopFile) {
    XdgDesktopRepository repo;
    auto item = repo.resolveDesktopFile("dolphin.desktop");
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->name, "Dolphin");
    EXPECT_EQ(item->iconName, "org.kde.dolphin");
    EXPECT_TRUE(item->id.find("org.kde.dolphin.desktop") != std::string::npos);
}

TEST(XdgDesktopRepositoryTest, ThemeIconProvider_ResolvesDolphinIcon) {
    int argc = 0;
    char *argv[] = { nullptr };
    std::unique_ptr<QGuiApplication> app;
    if (!qApp) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        app = std::make_unique<QGuiApplication>(argc, argv);
    }

    ThemeIconProvider provider;
    QSize size;

    // Test resolving with full reverse-DNS name
    QImage imgDolphin = provider.requestImage(QStringLiteral("org.kde.dolphin"), &size, QSize(64, 64));
    EXPECT_FALSE(imgDolphin.isNull());
    EXPECT_EQ(imgDolphin.width(), 64);
    EXPECT_EQ(imgDolphin.height(), 64);

    // Test resolving with shorthand "dolphin"
    QImage imgShorthand = provider.requestImage(QStringLiteral("dolphin"), &size, QSize(64, 64));
    EXPECT_FALSE(imgShorthand.isNull());
    EXPECT_EQ(imgShorthand.width(), 64);
    EXPECT_EQ(imgShorthand.height(), 64);

    // Test resolving with "dolphin.desktop"
    QImage imgDesktop = provider.requestImage(QStringLiteral("dolphin.desktop"), &size, QSize(64, 64));
    EXPECT_FALSE(imgDesktop.isNull());
    EXPECT_EQ(imgDesktop.width(), 64);
    EXPECT_EQ(imgDesktop.height(), 64);
}

TEST(XdgDesktopRepositoryTest, BuildLaunchCommand_WithSystemdScope_EscapesDockCGroupIntoAppSlice) {
    auto spec = XdgDesktopRepository::buildLaunchCommand("kitty --single-instance %U", false, true);
    ASSERT_TRUE(spec.usesSystemdScope);
    EXPECT_NE(spec.program.find("systemd-run"), std::string::npos);

    ASSERT_GE(spec.arguments.size(), 8u);
    EXPECT_EQ(spec.arguments[0], "--user");
    EXPECT_EQ(spec.arguments[1], "--scope");
    EXPECT_EQ(spec.arguments[2], "--collect");
    EXPECT_EQ(spec.arguments[3], "--quiet");
    EXPECT_EQ(spec.arguments[4], "--slice=app.slice");
    EXPECT_EQ(spec.arguments[5], "--");
    EXPECT_EQ(spec.arguments[6], "kitty");
    EXPECT_EQ(spec.arguments[7], "--single-instance");
}

TEST(XdgDesktopRepositoryTest, BuildLaunchCommand_WithDiscreteGpuAndSystemdScope_PropagatesPrimeEnvVars) {
    auto spec = XdgDesktopRepository::buildLaunchCommand("blender %f", true, true);
    ASSERT_TRUE(spec.usesSystemdScope);
    EXPECT_EQ(spec.envOverrides.size(), 4u);

    bool hasDriPrimeArg = false;
    bool hasNvPrimeArg = false;
    for (const auto &arg : spec.arguments) {
        if (arg == "--setenv=DRI_PRIME=1") hasDriPrimeArg = true;
        if (arg == "--setenv=__NV_PRIME_RENDER_OFFLOAD=1") hasNvPrimeArg = true;
    }
    EXPECT_TRUE(hasDriPrimeArg);
    EXPECT_TRUE(hasNvPrimeArg);
    EXPECT_EQ(spec.arguments.back(), "blender");
}

TEST(XdgDesktopRepositoryTest, BuildLaunchCommand_WithoutSystemdScope_FallsBackToDirectExecutable) {
    auto spec = XdgDesktopRepository::buildLaunchCommand("/usr/bin/dolphin --new-window %u", false, false);
    EXPECT_FALSE(spec.usesSystemdScope);
    EXPECT_EQ(spec.program, "/usr/bin/dolphin");
    ASSERT_EQ(spec.arguments.size(), 1u);
    EXPECT_EQ(spec.arguments[0], "--new-window");
}

TEST(XdgDesktopRepositoryTest, Launch_WithNonExistentExecutable_ReturnsFalseImmediately) {
    EXPECT_FALSE(XdgDesktopRepository::launch("definitely_missing_binary_xyz_987 --flag", false));
}
