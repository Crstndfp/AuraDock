#pragma once

#include "domain/entities/DockItem.hpp"
#include <string>
#include <vector>
#include <optional>

namespace cachy_dock::infrastructure::xdg {

class XdgDesktopRepository {
public:
    XdgDesktopRepository();
    ~XdgDesktopRepository();

    [[nodiscard]] std::vector<domain::entities::DockItem> getDefaultPinnedApps() const;
    [[nodiscard]] static std::optional<domain::entities::DockItem> resolveDesktopFile(const std::string& desktopPathOrName);

    static bool launch(const std::string& execCommand, bool useDiscreteGpu = false);
    static bool launchDesktopId(const std::string& desktopId, bool useDiscreteGpu = false);
};

} // namespace cachy_dock::infrastructure::xdg
