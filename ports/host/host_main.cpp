#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/aegis_core/aegis_core.hpp"
#include "ports/host/filesystem_app_package_source.hpp"
#include "ports/host/host_logger.hpp"
#include "shell/control/shell_input_model.hpp"

int main(int argc, char** argv) {
    aegis::ports::host::HostLogger logger;

    try {
        const std::string package_id = argc > 1 ? argv[1] : "mock_device_a";

        auto app_source = std::make_shared<aegis::ports::host::FilesystemAppPackageSource>(
            std::filesystem::path("apps"));

        aegis::core::AegisCore core(app_source, logger);
        core.boot(package_id);
        if (argc > 2 && std::string(argv[2]) == "--probe-foreground-events") {
            int step = 0;
            core.set_foreground_input_pollers(
                [&step]() -> std::optional<aegis::shell::ShellInputInvocation> {
                    if (step == 0) {
                        ++step;
                        return aegis::shell::make_page_command_invocation(
                            "demo_hello.main", "ping", "hello-0");
                    }
                    return std::nullopt;
                },
                [&step]() -> std::optional<aegis::shell::ShellNavigationAction> {
                    if (step == 1) {
                        ++step;
                        return aegis::shell::ShellNavigationAction::Select;
                    }
                    if (step == 2) {
                        ++step;
                        return aegis::shell::ShellNavigationAction::Back;
                    }
                    return std::nullopt;
                });
            core.run_app("demo_hello");
        } else if (argc > 2 && std::string(argv[2]) == "--shell-actions") {
            std::vector<aegis::shell::ShellNavigationAction> actions;
            for (int index = 3; index < argc; ++index) {
                aegis::shell::ShellNavigationAction action {};
                if (!aegis::shell::try_parse_shell_action(argv[index], action)) {
                    throw std::runtime_error("unknown shell action: " + std::string(argv[index]));
                }
                actions.push_back(action);
            }
            core.run_shell_action_sequence(actions);
        } else {
            const std::string app_id = argc > 2 ? argv[2] : "demo_info";
            core.run_app(app_id);
        }
        return 0;
    } catch (const std::exception& ex) {
        logger.error("fatal", ex.what());
        return 1;
    }
}
