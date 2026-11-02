#pragma once

#include "domain/entities/DockItem.hpp"
#include <string>
#include <vector>
#include <utility>
#include <optional>

namespace cachy_dock::infrastructure::xdg {

struct LaunchCommandSpec {
    std::string program;
    std::vector<std::string> arguments;
    std::vector<std::pair<std::string, std::string>> envOverrides;
    bool usesSystemdScope{false};
};

class XdgDesktopRepository {
public:
    XdgDesktopRepository();
    ~XdgDesktopRepository();

    [[nodiscard]] std::vector<domain::entities::DockItem> getDefaultPinnedApps() const;
    [[nodiscard]] static std::optional<domain::entities::DockItem> resolveDesktopFile(const std::string& desktopPathOrName);

    [[nodiscard]] static bool isSystemdUserSessionAvailable();
    [[nodiscard]] static LaunchCommandSpec buildLaunchCommand(
        const std::string& execCommand,
        bool useDiscreteGpu = false,
        bool isolateInSystemdScope = isSystemdUserSessionAvailable());

    static bool launch(const std::string& execCommand, bool useDiscreteGpu = false);
    static bool launchDesktopId(const std::string& desktopId, bool useDiscreteGpu = false);
};

} // namespace cachy_dock::infrastructure::xdg
