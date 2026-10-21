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
