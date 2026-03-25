#include <memory>
#include <string>
#include <vector>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>

#include <esp_rom_sys.h>

#include "core/aegis_core/aegis_core.hpp"
#include "ports/zephyr/zephyr_app_package_source.hpp"
#include "ports/zephyr/zephyr_appfs.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_descriptors.hpp"
#include "ports/zephyr/zephyr_board_support.hpp"
#include "ports/zephyr/zephyr_board_packages.hpp"
#include "ports/zephyr/zephyr_boot_log_logger.hpp"
#include "ports/zephyr/zephyr_build_config.hpp"
#include "ports/zephyr/zephyr_llext_adapter.hpp"
#include "ports/zephyr/zephyr_logger.hpp"
#include "ports/zephyr/zephyr_shell_display_adapter.hpp"
#include "ports/zephyr/zephyr_shell_input_adapter.hpp"
#include "runtime/loader/llext_loader_backend.hpp"

namespace aegis::ports::zephyr {

void run_shell_presentation_selftest(aegis::core::AegisCore& core, aegis::platform::Logger& logger) {
    const std::vector<aegis::shell::ShellNavigationAction> script {
        aegis::shell::ShellNavigationAction::OpenMenu,
        aegis::shell::ShellNavigationAction::Select,
        aegis::shell::ShellNavigationAction::MoveNext,
        aegis::shell::ShellNavigationAction::MovePrevious,
        aegis::shell::ShellNavigationAction::OpenSettings,
        aegis::shell::ShellNavigationAction::OpenNotifications,
        aegis::shell::ShellNavigationAction::Back,
    };

    logger.info("zephyr", "shell self-test begin");
    for (const auto action : script) {
        logger.info("zephyr", "shell self-test action=" + std::string(aegis::shell::to_string(action)));
        core.run_shell_action_sequence({action});
        k_sleep(K_MSEC(120));
    }
    core.run_shell_action_sequence({
        aegis::shell::ShellNavigationAction::OpenMenu,
    });
    logger.info("zephyr", "shell self-test reset to home");
    logger.info("zephyr", "shell self-test complete");
}

}  // namespace aegis::ports::zephyr

extern "C" int main(void) {
    esp_rom_printf("AEGIS ROM: main entered\n");
    printk("AEGIS TRACE: main entered\n");
    aegis::ports::zephyr::ZephyrLogger logger;
    esp_rom_printf("AEGIS ROM: logger ready\n");
    printk("AEGIS TRACE: logger ready\n");
    aegis::ports::zephyr::ZephyrBootLogLogger boot_logger(logger);
    settings_subsys_init();
    esp_rom_printf("AEGIS ROM: settings ready\n");
    printk("AEGIS TRACE: settings ready\n");
    aegis::ports::zephyr::ZephyrAppFs appfs(boot_logger);
    const bool appfs_ready = appfs.mount();
    printk("AEGIS TRACE: appfs mount=%d\n", appfs_ready ? 1 : 0);

    std::vector<std::string> app_package_roots;
    if (appfs_ready) {
        app_package_roots.push_back(appfs.apps_root());
    }
    printk("AEGIS TRACE: app roots prepared\n");
    printk("AEGIS TRACE: app roots count=%u first=%s\n",
           static_cast<unsigned int>(app_package_roots.size()),
           app_package_roots.empty() ? "<none>" : app_package_roots.front().c_str());

    auto app_source =
        std::make_shared<aegis::ports::zephyr::ZephyrAppPackageSource>(
            app_package_roots);
    printk("AEGIS TRACE: app source ready\n");

    boot_logger.info("zephyr", "bootstrapping Zephyr-backed Aegis target");
    std::string app_root_summary = "app package roots:";
    if (app_package_roots.empty()) {
        app_root_summary += " <none>";
    } else {
        for (const auto& root : app_package_roots) {
            app_root_summary += " ";
            app_root_summary += root;
        }
    }
    boot_logger.info("zephyr", app_root_summary);
    boot_logger.info("zephyr", std::string("appfs status: ") + (appfs_ready ? "mounted" : "unavailable"));
    printk("AEGIS TRACE: app root summary %s\n", app_root_summary.c_str());

    auto loader_backend = std::make_unique<aegis::runtime::LlextLoaderBackend>(
        boot_logger,
        std::make_unique<aegis::ports::zephyr::ZephyrLlextAdapter>(boot_logger));
    esp_rom_printf("AEGIS ROM: loader backend ready\n");
    printk("AEGIS TRACE: loader backend ready\n");

    const auto board_support =
        aegis::ports::zephyr::resolve_zephyr_board_support(aegis::ports::zephyr::kBootstrapDevicePackage,
                                                           boot_logger);
    const auto& board_descriptor = *board_support.descriptor;
    auto& board_runtime = *board_support.runtime;
    aegis::ports::zephyr::set_active_zephyr_board_runtime(&board_runtime);
    const auto board_config = board_descriptor.config;
    esp_rom_printf("AEGIS ROM: board config ready\n");
    printk("AEGIS TRACE: board config ready\n");
    const bool board_runtime_ready =
        aegis::ports::zephyr::initialize_board_runtime(aegis::ports::zephyr::kBootstrapDevicePackage,
                                                       boot_logger);
    board_runtime.log_state(board_descriptor.bringup.runtime_ready_stage);
    board_runtime.signal_boot_stage(board_descriptor.bringup.runtime_ready_signal_stage);
    printk("AEGIS TRACE: stage 1\n");
    aegis::ports::zephyr::ZephyrShellDisplayAdapter display_adapter(boot_logger,
                                                                    board_runtime,
                                                                    board_config);
    aegis::ports::zephyr::ZephyrShellInputAdapter input_adapter(boot_logger,
                                                                board_runtime,
                                                                board_config);
    boot_logger.attach_display(&display_adapter);
    printk("AEGIS TRACE: input adapter constructed\n");

    const bool display_ready = display_adapter.initialize();
    if (display_ready) {
        display_adapter.present_boot_log_screen("display-init");
        k_sleep(K_MSEC(3000));
    }
    board_runtime.signal_boot_stage(board_descriptor.bringup.display_ready_signal_stage);
    printk("AEGIS TRACE: display init=%d\n", display_ready ? 1 : 0);
    boot_logger.info("zephyr", std::string("display adapter state: ") +
                                 (display_ready ? "ready" : "missing") + " " +
                                 appfs.diagnostics_summary() +
                                 " board-runtime=" + (board_runtime_ready ? std::string("ready")
                                                                          : std::string("degraded")));
    if (display_ready) {
        display_adapter.present_boot_log_screen("display-ready");
        k_sleep(K_MSEC(3000));
    }

    aegis::core::AegisCore core(app_source,
                                boot_logger,
                                aegis::ports::zephyr::make_zephyr_board_packages(),
                                std::move(loader_backend));
    if (display_ready) {
        core.attach_shell_presentation_sink(&display_adapter);
        display_adapter.present_boot_log_screen("core-attach");
    }
    board_runtime.signal_boot_stage(board_descriptor.bringup.core_ready_signal_stage);
    esp_rom_printf("AEGIS ROM: core constructed\n");
    printk("AEGIS TRACE: core constructed\n");
    const bool input_ready = input_adapter.initialize();
    board_runtime.log_state(board_descriptor.bringup.input_ready_stage);
    board_runtime.signal_boot_stage(board_descriptor.bringup.input_ready_signal_stage);
    esp_rom_printf("AEGIS ROM: input init=%d\n", input_ready ? 1 : 0);
    printk("AEGIS TRACE: input init=%d\n", input_ready ? 1 : 0);
    if (display_ready) {
        display_adapter.present_boot_log_screen("input-ready");
    }
    core.boot(aegis::ports::zephyr::kBootstrapDevicePackage);
    board_runtime.signal_boot_stage(board_descriptor.bringup.interactive_ready_signal_stage);
    esp_rom_printf("AEGIS ROM: core boot complete\n");
    printk("AEGIS TRACE: core boot complete\n");
    boot_logger.info("zephyr",
                     "interactive adapters display=" + std::string(display_ready ? "ready" : "missing") +
                         " input=" + std::string(input_ready ? "ready" : "missing"));
    if (std::string(aegis::ports::zephyr::kBootSelftestAppId).empty()) {
        boot_logger.info("zephyr", "boot self-test disabled");
        printk("AEGIS TRACE: selftest disabled\n");
    } else {
        printk("AEGIS TRACE: selftest launching %s\n", aegis::ports::zephyr::kBootSelftestAppId);
        boot_logger.info("zephyr",
                         "boot self-test launching app=" +
                             std::string(aegis::ports::zephyr::kBootSelftestAppId));
        try {
            core.run_app(aegis::ports::zephyr::kBootSelftestAppId);
            printk("AEGIS TRACE: selftest completed %s\n", aegis::ports::zephyr::kBootSelftestAppId);
            boot_logger.info("zephyr",
                             "boot self-test completed app=" +
                                 std::string(aegis::ports::zephyr::kBootSelftestAppId));
        } catch (const std::exception& ex) {
            printk("AEGIS TRACE: selftest failed %s error=%s\n",
                   aegis::ports::zephyr::kBootSelftestAppId,
                   ex.what());
            boot_logger.info("zephyr",
                             "boot self-test failed app=" +
                                 std::string(aegis::ports::zephyr::kBootSelftestAppId) +
                                 " error=" + ex.what());
        }
    }
    if (board_descriptor.bringup.run_shell_selftest) {
        boot_logger.info("zephyr", "shell self-test launching");
        aegis::ports::zephyr::run_shell_presentation_selftest(core, boot_logger);
    } else {
        boot_logger.info("zephyr", "shell self-test skipped by board bringup plan");
    }
    boot_logger.attach_display(nullptr);
    boot_logger.info("zephyr", "boot log mirror detached for interactive runtime");
    input_adapter.enable_interactive_mode();
    boot_logger.info("zephyr", "interactive input enabled");
    printk("AEGIS TRACE: entering loop\n");

    int heartbeat_ticks = 0;
    int loop_heartbeats = 0;
    while (true) {
        if (const auto action = input_adapter.poll_action(); action.has_value()) {
            core.run_shell_action_sequence({*action});
        }
        ++heartbeat_ticks;
        if (heartbeat_ticks >= 50) {
            heartbeat_ticks = 0;
            ++loop_heartbeats;
            esp_rom_printf("AEGIS HEARTBEAT %d\n", loop_heartbeats);
            printk("AEGIS TRACE: heartbeat=%d\n", loop_heartbeats);
            board_runtime.heartbeat_pulse();
            if (loop_heartbeats <= 5 || (loop_heartbeats % 25) == 0) {
                board_runtime.log_state("loop-heartbeat");
            }
        }
        k_sleep(K_MSEC(8));
    }

    return 0;
}
