#pragma once

#ifndef AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK
#define AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK 1
#endif

#ifndef AEGIS_ZEPHYR_APPFS_MOUNT_POINT
#define AEGIS_ZEPHYR_APPFS_MOUNT_POINT "/lfs"
#endif

#ifndef AEGIS_ZEPHYR_APPFS_APPS_ROOT
#define AEGIS_ZEPHYR_APPFS_APPS_ROOT "/lfs/apps"
#endif

#ifndef AEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID
#define AEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID ""
#endif

#ifndef AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE
#define AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE "zephyr_tlora_pager_sx1262"
#endif

namespace aegis::ports::zephyr {

inline constexpr bool kCompiledAppFallbackEnabled =
    AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK != 0;
inline constexpr const char* kAppFsMountPoint = AEGIS_ZEPHYR_APPFS_MOUNT_POINT;
inline constexpr const char* kAppFsAppsRoot = AEGIS_ZEPHYR_APPFS_APPS_ROOT;
inline constexpr const char* kBootSelftestAppId = AEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID;
inline constexpr const char* kBootstrapDevicePackage = AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE;

}  // namespace aegis::ports::zephyr
