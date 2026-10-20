#include <gtest/gtest.h>
#include "infrastructure/xdg/XdgDesktopRepository.hpp"

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
