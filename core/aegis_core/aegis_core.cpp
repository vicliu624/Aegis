#include "core/aegis_core/aegis_core.hpp"

#include <algorithm>
#include <stdexcept>

#include "device/packages/mock_device_packages.hpp"
#include "runtime/fault/fault.hpp"
#include "runtime/host_api/host_api.hpp"
#include "runtime/loader/stub_loader_backend.hpp"
#include "runtime/supervisor/app_supervisor.hpp"
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

void AegisCore::set_foreground_input_pollers(
    std::function<std::optional<shell::ShellInputInvocation>()> ui_invocation_poller,
    std::function<std::optional<shell::ShellNavigationAction>()> routed_action_poller) {
    foreground_ui_invocation_poller_ = std::move(ui_invocation_poller);
    foreground_routed_action_poller_ = std::move(routed_action_poller);
}

void AegisCore::boot(const std::string& package_id) {
    logger_.info("core", "boot begin package=" + package_id);
    boot_artifacts_ = boot_.boot(package_id);
    logger_.info("core", "boot artifacts ready profile=" + boot_artifacts_->device_profile.device_id);
    if (boot_artifacts_->service_bindings.text_input()) {
        logger_.info("core", "assign shell text input focus");
        boot_artifacts_->service_bindings.text_input()->assign_shell_focus();
    }
    shell_.bind_services(boot_artifacts_->service_bindings);
    logger_.info("core", "configure shell for device");
    shell_.configure_for_device(boot_artifacts_->device_profile,
                                boot_artifacts_->shell_surface_profile);
    logger_.info("core", "present shell home");
    shell_.present_home();
    logger_.info("core", "present system surfaces");
    shell_.present_system_surfaces();
    const auto catalog = catalog_builder_.build(boot_artifacts_->app_registry,
                                                boot_artifacts_->device_profile,
                                                admission_policy_,
                                                compatibility_evaluator_,
                                                runtime_loader_,
                                                runtime_policy_,
                                                AEGIS_APP_ABI_VERSION_V1,
                                                active_app_ids());
    shell_.present_launcher_catalog(catalog);
    logger_.info("core",
                 "launcher catalog hydrated entries=" +
                     std::to_string(catalog.entries().size()));
    logger_.info("core",
                 "app registry ready entries=" +
                     std::to_string(boot_artifacts_->app_registry.apps().size()) +
                     " catalog-materialization=deferred");
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
        logger_.info("core", "shell action begin " + std::string(shell::to_string(action)));
        const auto launch_request = shell_.handle_action(action);
        logger_.info("core", "shell action end " + std::string(shell::to_string(action)));
        if (launch_request) {
            logger_.info("core", "shell action launch request " + *launch_request);
            run_app(*launch_request);
        }
    }
}

void AegisCore::run_shell_input_sequence(const std::vector<shell::ShellInputInvocation>& invocations) {
    if (!booted_) {
        throw std::runtime_error("core must boot before shell inputs");
    }

    for (const auto& invocation : invocations) {
        if (invocation.target == shell::ShellInputInvocationTarget::SystemAction) {
            logger_.info("core",
                         "shell system input begin " +
                             std::string(shell::to_string(invocation.system_action)));
        } else {
            logger_.info("core",
                         "shell page command begin page=" +
                             std::string(shell::invocation_page_id(invocation)) + " command=" +
                             std::string(shell::invocation_page_command_id(invocation)));
        }

        const auto launch_request = shell_.handle_input_invocation(invocation);

        if (invocation.target == shell::ShellInputInvocationTarget::SystemAction) {
            logger_.info("core",
                         "shell system input end " +
                             std::string(shell::to_string(invocation.system_action)));
        } else {
            logger_.info("core",
                         "shell page command end page=" +
                             std::string(shell::invocation_page_id(invocation)) + " command=" +
                             std::string(shell::invocation_page_command_id(invocation)));
        }

        if (launch_request) {
            logger_.info("core", "shell action launch request " + *launch_request);
            run_app(*launch_request);
        }
    }
}

void AegisCore::run_descriptor(const AppDescriptor& descriptor) {
    shell_.clear_app_foreground_state();
    shell_.enter_app(descriptor.manifest.app_id);
    logger_.info("core", "starting app " + descriptor.manifest.app_id);
    active_sessions_.push_back(descriptor.manifest.app_id);
    AppSession session(descriptor);
    runtime::AppSupervisor supervisor;
    session.transition_to(runtime::AppLifecycleState::LoadRequested);
    runtime::HostApi host_api(boot_artifacts_->device_profile,
                              boot_artifacts_->service_bindings,
                              ownership_,
                              descriptor.app_dir,
                              session.id(),
                              descriptor.manifest.requested_permissions,
                              &session.instance().quota_ledger(),
                              [this](const std::string& root_name) {
                                  shell_.set_app_foreground_root(root_name);
                              },
                              [this](const runtime::ForegroundPagePresentation& page) {
                                  shell_.set_app_foreground_page(page);
                              },
                              [this]() -> std::optional<shell::ShellInputInvocation> {
                                  if (!foreground_ui_invocation_poller_) {
                                      return std::nullopt;
                                  }
                                  return foreground_ui_invocation_poller_();
                              },
                              [this]() -> std::optional<shell::ShellNavigationAction> {
                                  if (!foreground_routed_action_poller_) {
                                      return std::nullopt;
                                  }
                                  return foreground_routed_action_poller_();
                              },
                              [this](const std::string& tag, const std::string& message) {
                                  shell_.append_app_foreground_log(tag, message);
                              });

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

        if (boot_artifacts_->service_bindings.text_input() &&
            std::find(descriptor.manifest.requested_permissions.begin(),
                      descriptor.manifest.requested_permissions.end(),
                      AppPermissionId::TextInputFocus) !=
                descriptor.manifest.requested_permissions.end()) {
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
        supervisor.note_fault(session.instance(),
                              runtime::AppFaultRecord {
                                  .kind = runtime::AppFaultKind::UnhandledException,
                                  .detail = ex.what(),
                                  .lifecycle_state = session.state(),
                                  .fatal = true,
                                  .strike_cost = 1,
                              });
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

        const auto recovery_plan = supervisor.build_recovery_plan(session.instance());
        logger_.info("core",
                     "recovery plan terminate=" +
                         std::string(recovery_plan.terminate_instance ? "yes" : "no") +
                         " quarantine=" +
                         std::string(recovery_plan.quarantine_instance ? "yes" : "no") +
                         " actions=" + std::to_string(recovery_plan.actions.size()));
    }

    active_sessions_.pop_back();
    if (boot_artifacts_->service_bindings.text_input()) {
        boot_artifacts_->service_bindings.text_input()->assign_shell_focus();
    }
    shell_.return_from_app(descriptor.manifest.app_id);
    shell_.clear_app_foreground_state();
    logger_.info("core",
                 "lifecycle final state=" + std::string(runtime::to_string(session.state())));
    logger_.info("core", "lifecycle history=" + format_lifecycle_history(session));
}

std::vector<std::string> AegisCore::active_app_ids() const {
    return active_sessions_;
}

}  // namespace aegis::core
