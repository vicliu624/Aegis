#pragma once

#include <string>

#include "platform/logging/logger.hpp"

namespace aegis::ports::zephyr {

class ZephyrAppFs {
public:
    explicit ZephyrAppFs(platform::Logger& logger);
    ~ZephyrAppFs();

    ZephyrAppFs(const ZephyrAppFs&) = delete;
    ZephyrAppFs& operator=(const ZephyrAppFs&) = delete;

    [[nodiscard]] bool mount();
    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] std::string mount_point() const;
    [[nodiscard]] std::string apps_root() const;
    [[nodiscard]] std::size_t app_directory_count() const;
    [[nodiscard]] std::string diagnostics_summary() const;

private:
    [[nodiscard]] bool ensure_directory(const std::string& path) const;
    [[nodiscard]] std::size_t count_directories(const std::string& path) const;

    platform::Logger& logger_;
    bool mounted_ {false};
};

}  // namespace aegis::ports::zephyr
