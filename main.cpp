#include <filesystem>
#include <iostream>
#include <string>

#include "core/aegis_core/aegis_core.hpp"

int main(int argc, char** argv) {
    try {
        const std::string package_id = argc > 1 ? argv[1] : "mock_device_a";
        const std::string app_id = argc > 2 ? argv[2] : "demo_info";
        aegis::core::AegisCore core(std::filesystem::path("apps"));
        core.boot(package_id);
        core.run_app(app_id);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[fatal] " << ex.what() << '\n';
        return 1;
    }
}
