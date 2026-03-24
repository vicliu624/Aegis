#include "core/aegis_core/aegis_core.hpp"

#include <stdexcept>

#include "device/packages/mock_device_packages.hpp"
#include "runtime/host_api/host_api.hpp"
#include "runtime/loader/stub_loader_backend.hpp"
#include "sdk/include/aegis/abi.h"

namespace aegis::core {

namespace {

std::string format_lifecycle_history(const AppSession& session) {
    std::string text;
    const auto& history = session.history();
    for (std::size_t index = 0; index < history.size(); ++index) {
        if (index > 0) {
            text += " -> ";
        }
        text += runtime::to_string(history[index]);
    }
    return text;
}

}  // namespace

AegisCore::AegisCore(std::shared_ptr<AppPackageSource> app_source, platform::Logger& logger)
    : AegisCore(std::move(app_source),
                logger,
                device::make_mock_board_packages(),
                std::make_unique<runtime::StubLoaderBackend>(logger)) {}

AegisCore::AegisCore(std::shared_ptr<AppPackageSource> app_source,
                     platform::Logger& logger,
                     std::vector<device::BoardPackagePtr> board_packages)
    : AegisCore(std::move(app_source),
                logger,
                std::move(board_packages),
                std::make_unique<runtime::StubLoaderBackend>(logger)) {}

AegisCore::AegisCore(std::shared_ptr<AppPackageSource> app_source,
                     platform::Logger& logger,
                     std::vector<device::BoardPackagePtr> board_packages,
                     runtime::LoaderBackendPtr loader_backend)
    : app_source_(std::move(app_source)),
      logger_(logger),
      runtime_policy_ {.background_execution_mode = BackgroundExecutionMode::Unsupported,
                       .permissions =
                           {
                               {.permission = AppPermissionId::UiSurface,
                                .state = PermissionGrantState::Granted,
                                .reason = "runtime permits app-owned foreground UI roots"},
                               {.permission = AppPermissionId::TimerControl,
                                .state = PermissionGrantState::Granted,
                                .reason = "session-scoped timers are supported"},
                               {.permission = AppPermissionId::SettingsWrite,
                                .state = PermissionGrantState::Granted,
                                .reason = "settings writes are routed through system policy"},
                               {.permission = AppPermissionId::NotificationPost,
                                .state = PermissionGrantState::Granted,
                                .reason = "notifications remain shell-mediated"},
                               {.permission = AppPermissionId::StorageAccess,
                                .state = PermissionGrantState::Granted,
                                .reason = "storage access remains namespaced by the system"},
                               {.permission = AppPermissionId::TextInputFocus,
                                .state = PermissionGrantState::Granted,
                                .reason =
                                    "text input focus remains session-scoped and shell-recoverable"},
                               {.permission = AppPermissionId::RadioUse,
                                .state = PermissionGrantState::Granted,
                                .reason = "radio service remains capability-governed"},
                               {.permission = AppPermissionId::GpsUse,
                                .state = PermissionGrantState::Granted,
                                .reason = "gps service remains capability-governed"},
                               {.permission = AppPermissionId::AudioUse,
                                .state = PermissionGrantState::Granted,
                                .reason = "audio service remains capability-governed"},
                               {.permission = AppPermissionId::HostlinkUse,
                                .state = PermissionGrantState::Denied,
                                .reason = "hostlink transport remains reserved until runtime bridge policy is formalized"},
                           }},
      boot_(app_source_, logger_, std::move(board_packages)),
      runtime_loader_(logger_, std::move(loader_backend)),
      shell_(logger_) {}

void AegisCore::attach_shell_presentation_sink(shell::ShellPresentationSink* sink) {
    shell_.set_presentation_sink(sink);
}

void AegisCore::boot(const std::string& package_id) {
    logger_.info("core", "boot begin package=" + package_id);
    boot_artifacts_ = boot_.boot(package_id);
    logger_.info("core", "boot artifacts ready profile=" + boot_artifacts_->device_profile.device_id);
    if (boot_artifacts_->service_bindings.text_input()) {
        logger_.info("core", "assign shell text input focus");
        boot_artifacts_->service_bindings.text_input()->assign_shell_focus();
    }
    logger_.info("core", "configure shell for device");
    shell_.configure_for_device(boot_artifacts_->device_profile,
                                boot_artifacts_->shell_surface_profile);
    logger_.info("core", "present shell home");
    shell_.present_home();
    logger_.info("core", "present system surfaces");
    shell_.present_system_surfaces();
    logger_.info("core", "build launcher catalog");
    const auto catalog = catalog_builder_.build(boot_artifacts_->app_registry,
                                                boot_artifacts_->device_profile,
                                                admission_policy_,
                                                compatibility_evaluator_,
                                                runtime_loader_,
                                                runtime_policy_,
                                                AEGIS_APP_ABI_VERSION_V1,
                                                active_app_ids());
    logger_.info("core", "load launcher catalog entries=" + std::to_string(catalog.entries().size()));
    shell_.load_launcher_catalog(catalog);
    logger_.info("core", "return shell to home after boot catalog load");
    shell_.present_home();
    booted_ = true;
    logger_.info("core", "boot complete");
}

void AegisCore::run_first_compatible_app() {
    if (!booted_) {
        throw std::runtime_error("core must boot before starting apps");
    }
    if (!boot_artifacts_) {
        throw std::runtime_error("boot artifacts missing");
    }

    const AppDescriptor* selected = nullptr;
    for (const auto& app : boot_artifacts_->app_registry.apps()) {
        const auto admission = admission_policy_.evaluate(
            app,
            AppAdmissionContext {.registry = boot_artifacts_->app_registry,
                                 .profile = boot_artifacts_->device_profile,
                                 .runtime_abi_version = AEGIS_APP_ABI_VERSION_V1,
                                 .runtime_policy = runtime_policy_,
                                 .active_app_ids = active_app_ids()});
        if (admission.allowed) {
            selected = &app;
            break;
        }
    }

    if (!selected) {
        logger_.info("core", "no compatible apps for profile " + boot_artifacts_->device_profile.device_id);
        return;
    }

    run_descriptor(*selected);
}

void AegisCore::run_app(const std::string& app_id) {
    if (!booted_) {
        throw std::runtime_error("core must boot before starting apps");
    }
    if (!boot_artifacts_) {
        throw std::runtime_error("boot artifacts missing");
    }

    const auto* descriptor = boot_artifacts_->app_registry.find_by_id(app_id);
    if (!descriptor) {
        throw std::runtime_error("app not found: " + app_id);
    }
    const auto admission =
        admission_policy_.evaluate(*descriptor,
                                   AppAdmissionContext {.registry = boot_artifacts_->app_registry,
                                                        .profile = boot_artifacts_->device_profile,
                                                        .runtime_abi_version =
                                                            AEGIS_APP_ABI_VERSION_V1,
                                                        .runtime_policy = runtime_policy_,
                                                        .active_app_ids = active_app_ids()});
    if (!admission.allowed) {
        throw std::runtime_error("app admission failed: " + app_id + " (" + admission.reason + ")");
    }
    if (!runtime_loader_.can_resolve_entry_symbol(descriptor->manifest.entry_symbol)) {
        throw std::runtime_error("app entry symbol unavailable: " + descriptor->manifest.entry_symbol);
    }

    run_descriptor(*descriptor);
}

void AegisCore::run_shell_action_sequence(const std::vector<shell::ShellNavigationAction>& actions) {
    if (!booted_) {
        throw std::runtime_error("core must boot before shell actions");
    }
    for (const auto action : actions) {
        const auto launch_request = shell_.handle_action(action);
        if (launch_request) {
            run_app(*launch_request);
        }
    }
}

void AegisCore::run_descriptor(const AppDescriptor& descriptor) {
    shell_.enter_app(descriptor.manifest.app_id);
    logger_.info("core", "starting app " + descriptor.manifest.app_id);
    active_sessions_.push_back(descriptor.manifest.app_id);
    AppSession session(descriptor);
    session.transition_to(runtime::AppLifecycleState::LoadRequested);
    runtime::HostApi host_api(boot_artifacts_->device_profile,
                              boot_artifacts_->service_bindings,
                              ownership_,
                              session.id(),
                              descriptor.manifest.requested_permissions);

    bool loaded = false;
    bool torn_down = false;
    try {
        const auto prepare_result = runtime_loader_.prepare(session);
        if (!prepare_result.ok) {
            throw std::runtime_error("prepare failed: " + prepare_result.detail);
        }

        const auto load_result = runtime_loader_.load(session);
        loaded = load_result.ok;
        if (!load_result.ok) {
            throw std::runtime_error("load failed: " + load_result.detail);
        }

        if (boot_artifacts_->service_bindings.text_input()) {
            const auto status = host_api.request_text_input_focus();
            logger_.info("core",
                         std::string("text input foreground grant=") +
                             (status == AEGIS_HOST_STATUS_OK ? "granted" : "unavailable"));
        }

        const auto bringup_result = runtime_loader_.bringup(session, host_api);
        if (!bringup_result.ok) {
            logger_.info("core", "app bringup reported failure: " + bringup_result.detail);
        }

        const auto teardown_result = runtime_loader_.teardown(session, ownership_);
        torn_down = teardown_result.ok;
        if (!teardown_result.ok) {
            throw std::runtime_error("teardown failed: " + teardown_result.detail);
        }

        const auto unload_result = runtime_loader_.unload(session);
        if (!unload_result.ok) {
            throw std::runtime_error("unload failed: " + unload_result.detail);
        }
    } catch (const std::exception& ex) {
        logger_.info("core", "app failure path: " + std::string(ex.what()));
        if (loaded && !torn_down &&
            session.state() != runtime::AppLifecycleState::TornDown &&
            session.state() != runtime::AppLifecycleState::Unloaded &&
            session.state() != runtime::AppLifecycleState::ReturnedToShell) {
            try {
                const auto teardown_result = runtime_loader_.teardown(session, ownership_);
                torn_down = teardown_result.ok;
            } catch (const std::exception& teardown_ex) {
                logger_.info("core", "teardown recovery failed: " + std::string(teardown_ex.what()));
            }
        }
        if (loaded &&
            session.state() != runtime::AppLifecycleState::Unloaded &&
            session.state() != runtime::AppLifecycleState::ReturnedToShell) {
            try {
                runtime_loader_.unload(session);
            } catch (const std::exception& unload_ex) {
                logger_.info("core", "unload recovery failed: " + std::string(unload_ex.what()));
            }
        }
        try {
            session.recover_to_shell();
        } catch (const std::exception& recovery_ex) {
            logger_.info("core", "session recovery failed: " + std::string(recovery_ex.what()));
        }
    }

    active_sessions_.pop_back();
    if (boot_artifacts_->service_bindings.text_input()) {
        boot_artifacts_->service_bindings.text_input()->assign_shell_focus();
    }
    shell_.return_from_app(descriptor.manifest.app_id);
    logger_.info("core",
                 "lifecycle final state=" + std::string(runtime::to_string(session.state())));
    logger_.info("core", "lifecycle history=" + format_lifecycle_history(session));
}

std::vector<std::string> AegisCore::active_app_ids() const {
    return active_sessions_;
}

}  // namespace aegis::core
