#pragma once

#include <string>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrStorageService : public IStorageService {
public:
    explicit ZephyrStorageService(std::string mount_root = "/");
    ZephyrStorageService(std::string mount_root, ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string describe_backend() const override;

private:
    std::string mount_root_;
    ports::zephyr::ZephyrBoardBackendConfig config_;
};

}  // namespace aegis::services
