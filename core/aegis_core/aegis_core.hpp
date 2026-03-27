#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "core/app_registry/app_package_source.hpp"
#include "core/app_registry/app_admission_policy.hpp"
#include "core/app_registry/app_catalog_builder.hpp"
#include "core/app_registry/app_compatibility_evaluator.hpp"
#include "core/app_registry/app_runtime_policy.hpp"
#include "core/boot/boot_coordinator.hpp"
#include "platform/logging/logger.hpp"
#include "runtime/loader/runtime_loader.hpp"
#include "runtime/ownership/resource_ownership_table.hpp"
#include "shell/aegis_shell.hpp"
#include "shell/control/shell_input_model.hpp"
#include "shell/presentation/shell_presentation_sink.hpp"

namespace aegis::core {

class AegisCore {
public:
    AegisCore(std::shared_ptr<AppPackageSource> app_source, platform::Logger& logger);
    AegisCore(std::shared_ptr<AppPackageSource> app_source,
              platform::Logger& logger,
              std::vector<device::BoardPackagePtr> board_packages);
    AegisCore(std::shared_ptr<AppPackageSource> app_source,
              platform::Logger& logger,
              std::vector<device::BoardPackagePtr> board_packages,
              runtime::LoaderBackendPtr loader_backend);

    void boot(const std::string& package_id);
    void attach_shell_presentation_sink(shell::ShellPresentationSink* sink);
    void set_foreground_input_pollers(
        std::function<std::optional<shell::ShellInputInvocation>()> ui_invocation_poller,
        std::function<std::optional<shell::ShellNavigationAction>()> routed_action_poller);
    void run_first_compatible_app();
    void run_app(const std::string& app_id);
    void run_shell_action_sequence(const std::vector<shell::ShellNavigationAction>& actions);
    void run_shell_input_sequence(const std::vector<shell::ShellInputInvocation>& invocations);

private:
    void run_descriptor(const AppDescriptor& descriptor);
    [[nodiscard]] std::vector<std::string> active_app_ids() const;

    std::shared_ptr<AppPackageSource> app_source_;
    platform::Logger& logger_;
    AppRuntimePolicy runtime_policy_;
    AppAdmissionPolicy admission_policy_;
    AppCompatibilityEvaluator compatibility_evaluator_;
    AppCatalogBuilder catalog_builder_;
    BootCoordinator boot_;
    runtime::RuntimeLoader runtime_loader_;
    runtime::ResourceOwnershipTable ownership_;
    shell::AegisShell shell_;
    std::optional<BootArtifacts> boot_artifacts_;
    std::vector<std::string> active_sessions_;
    std::function<std::optional<shell::ShellInputInvocation>()> foreground_ui_invocation_poller_;
    std::function<std::optional<shell::ShellNavigationAction>()> foreground_routed_action_poller_;
    bool booted_ {false};
};

}  // namespace aegis::core
