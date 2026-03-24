#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "core/boot/boot_coordinator.hpp"
#include "runtime/loader/runtime_loader.hpp"
#include "runtime/ownership/resource_ownership_table.hpp"
#include "shell/aegis_shell.hpp"

namespace aegis::core {

class AegisCore {
public:
    explicit AegisCore(std::filesystem::path apps_root);

    void boot(const std::string& package_id);
    void run_first_compatible_app();
    void run_app(const std::string& app_id);

private:
    void run_descriptor(const AppDescriptor& descriptor);

    std::filesystem::path apps_root_;
    BootCoordinator boot_;
    runtime::RuntimeLoader runtime_loader_;
    runtime::ResourceOwnershipTable ownership_;
    shell::AegisShell shell_;
    std::optional<BootArtifacts> boot_artifacts_;
    bool booted_ {false};
};

}  // namespace aegis::core
