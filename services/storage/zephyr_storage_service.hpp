#pragma once

#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrStorageService : public IStorageService {
public:
    explicit ZephyrStorageService(std::string mount_root = "/");

    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string describe_backend() const override;

private:
    std::string mount_root_;
};

}  // namespace aegis::services
