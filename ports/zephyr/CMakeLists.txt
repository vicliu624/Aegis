cmake_minimum_required(VERSION 3.20)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

option(AEGIS_ZEPHYR_AUTO_APPLY_LOCAL_PATCHES
    "Automatically apply required local source patches to the active Zephyr tree during configure." ON)
option(AEGIS_ZEPHYR_ALLOW_UNSUPPORTED_LOCAL_PATCH_VERSION
    "Allow local Zephyr patch helpers to run against an unvalidated Zephyr version." OFF)

if(NOT DEFINED ENV{ZEPHYR_BASE} OR "$ENV{ZEPHYR_BASE}" STREQUAL "")
    message(FATAL_ERROR "ZEPHYR_BASE must be set before configuring the Zephyr target")
endif()

set(AEGIS_ZEPHYR_LOCAL_PATCH_HELPER
    "${CMAKE_CURRENT_LIST_DIR}/scripts/ensure_xtensa_rtld_patch.py")
set(AEGIS_ZEPHYR_LOCAL_PATCH_MODE "check")
if(AEGIS_ZEPHYR_AUTO_APPLY_LOCAL_PATCHES)
    set(AEGIS_ZEPHYR_LOCAL_PATCH_MODE "apply")
endif()

set(AEGIS_ZEPHYR_LOCAL_PATCH_EXTRA_ARGS "")
if(AEGIS_ZEPHYR_ALLOW_UNSUPPORTED_LOCAL_PATCH_VERSION)
    list(APPEND AEGIS_ZEPHYR_LOCAL_PATCH_EXTRA_ARGS --allow-unsupported-version)
endif()

execute_process(
    COMMAND ${Python3_EXECUTABLE}
        "${AEGIS_ZEPHYR_LOCAL_PATCH_HELPER}"
        --zephyr-base "$ENV{ZEPHYR_BASE}"
        --mode "${AEGIS_ZEPHYR_LOCAL_PATCH_MODE}"
        ${AEGIS_ZEPHYR_LOCAL_PATCH_EXTRA_ARGS}
    RESULT_VARIABLE AEGIS_ZEPHYR_LOCAL_PATCH_RC
    OUTPUT_VARIABLE AEGIS_ZEPHYR_LOCAL_PATCH_STDOUT
    ERROR_VARIABLE AEGIS_ZEPHYR_LOCAL_PATCH_STDERR
    COMMAND_ECHO STDOUT)

if(NOT AEGIS_ZEPHYR_LOCAL_PATCH_RC EQUAL 0)
    message(FATAL_ERROR
        "Failed to verify/apply required local Zephyr patches.\n"
        "stdout:\n${AEGIS_ZEPHYR_LOCAL_PATCH_STDOUT}\n"
        "stderr:\n${AEGIS_ZEPHYR_LOCAL_PATCH_STDERR}\n"
        "See ports/zephyr/patches/README.md for context.")
endif()

if(NOT "${AEGIS_ZEPHYR_LOCAL_PATCH_STDOUT}" STREQUAL "")
    message(STATUS "${AEGIS_ZEPHYR_LOCAL_PATCH_STDOUT}")
endif()
if(NOT "${AEGIS_ZEPHYR_LOCAL_PATCH_STDERR}" STREQUAL "")
    message(STATUS "${AEGIS_ZEPHYR_LOCAL_PATCH_STDERR}")
endif()

set(AEGIS_ZEPHYR_BOARD_FAMILY "generic")
set(AEGIS_ZEPHYR_BOARD_EXTRA_CONF "")
if(AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE STREQUAL "zephyr_tlora_pager_sx1262")
    set(AEGIS_ZEPHYR_BOARD_FAMILY "tlora_pager")
    set(AEGIS_ZEPHYR_BOARD_EXTRA_CONF
        "${CMAKE_CURRENT_LIST_DIR}/boards/tlora_pager/prj.conf")
elseif(AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE STREQUAL "zephyr_tdeck_sx1262")
    set(AEGIS_ZEPHYR_BOARD_FAMILY "tdeck")
    set(AEGIS_ZEPHYR_BOARD_EXTRA_CONF
        "${CMAKE_CURRENT_LIST_DIR}/boards/tdeck/prj.conf")
endif()

set(CONF_FILE "${CMAKE_CURRENT_LIST_DIR}/prj.conf")
if(NOT "${AEGIS_ZEPHYR_BOARD_EXTRA_CONF}" STREQUAL "")
    list(APPEND CONF_FILE "${AEGIS_ZEPHYR_BOARD_EXTRA_CONF}")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aegis_zephyr_target)

option(AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK "Keep compiled-in app contracts for bootstrap." ON)
option(AEGIS_ZEPHYR_ENABLE_APP_MODULES "Build standalone Zephyr app modules alongside the resident core." ON)
set(AEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID "" CACHE STRING
    "Optional app id auto-launched once after boot to validate launch/return over serial logs. Empty disables the self-test.")

set(AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR "${CMAKE_BINARY_DIR}/aegis/apps" CACHE PATH
    "Directory used to stage Zephyr app module artifacts")
set(AEGIS_ZEPHYR_APP_DEPLOY_ROOT "${CMAKE_BINARY_DIR}/deploy/lfs/apps" CACHE PATH
    "Directory used to assemble deployable Zephyr app packages")
set(AEGIS_ZEPHYR_APPFS_IMAGE_OUTPUT "${CMAKE_BINARY_DIR}/deploy/appfs.bin" CACHE FILEPATH
    "Output path for the packaged Zephyr appfs image")
set(AEGIS_ZEPHYR_APPFS_LAYOUT_JSON "${CMAKE_BINARY_DIR}/deploy/appfs-layout.json" CACHE FILEPATH
    "Output path for the packaged Zephyr appfs layout manifest")
set(AEGIS_ZEPHYR_APPFS_BLOCK_SIZE "4096" CACHE STRING
    "LittleFS block size used when assembling the appfs image")
set(AEGIS_ZEPHYR_APPFS_PAGE_SIZE "256" CACHE STRING
    "LittleFS page size used when assembling the appfs image")
set(AEGIS_ZEPHYR_APPFS_IMAGE_TOOL "" CACHE FILEPATH
    "Path to mklittlefs used to build a board-flashable appfs image")
set(AEGIS_ZEPHYR_APPFS_TOOL_CACHE_DIR "${CMAKE_BINARY_DIR}/tools/mklittlefs" CACHE PATH
    "Directory used to cache an auto-fetched mklittlefs binary")
option(AEGIS_ZEPHYR_APPFS_AUTO_FETCH_TOOL
    "Automatically fetch a host mklittlefs binary when no explicit tool path is configured." ON)
set(AEGIS_ZEPHYR_FLASH_TOOL "" CACHE FILEPATH
    "Optional flash tool executable used when writing appfs.bin to storage_partition")
set(AEGIS_ZEPHYR_FLASH_CHIP "esp32s3" CACHE STRING
    "Chip family used by the appfs flash helper")
set(AEGIS_ZEPHYR_FLASH_BAUD "921600" CACHE STRING
    "Baud rate used by the appfs flash helper")
set(AEGIS_ZEPHYR_FLASH_PORT "" CACHE STRING
    "Serial port used by the appfs flash helper")
set(AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE "zephyr_tlora_pager_sx1262" CACHE STRING
    "Board package id bootstrapped by the resident Aegis Zephyr target")

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../..
    ${CMAKE_CURRENT_LIST_DIR}/../../sdk/include)

target_compile_definitions(app PRIVATE
    AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK=$<BOOL:${AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK}>
    AEGIS_ZEPHYR_APPFS_MOUNT_POINT=\"/lfs\"
    AEGIS_ZEPHYR_APPFS_APPS_ROOT=\"/lfs/apps\"
    AEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID=\"${AEGIS_ZEPHYR_BOOT_SELFTEST_APP_ID}\"
    AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE=\"${AEGIS_ZEPHYR_BOOTSTRAP_DEVICE_PACKAGE}\"
    AEGIS_ZEPHYR_BOARD_FAMILY=\"${AEGIS_ZEPHYR_BOARD_FAMILY}\")

if(AEGIS_ZEPHYR_BOARD_FAMILY STREQUAL "tlora_pager")
    target_compile_definitions(app PRIVATE AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY=1)
elseif(AEGIS_ZEPHYR_BOARD_FAMILY STREQUAL "tdeck")
    target_compile_definitions(app PRIVATE AEGIS_ZEPHYR_HAS_TDECK_FAMILY=1)
endif()

target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/input_tca8418_keypad.c
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_appfs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_boot_log_logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_bluetooth_support.c
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_app_package_source.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_board_backend_config.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_board_descriptors.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_board_runtime.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_board_packages.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_board_support.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_lvgl_assets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_lvgl_settings_icon_asset.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_lvgl_status_icons_asset.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_lvgl_shell_ui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_shell_display_backend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_shell_input_backend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_shell_display_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_shell_input_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/aegis_core/aegis_core.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_admission_policy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_catalog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_catalog_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_compatibility_evaluator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_manifest.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_permissions.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_runtime_policy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_registry/app_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/app_session/app_session.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/boot/boot_coordinator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../core/device_registry/device_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../device/common/binding/mock_services.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../device/common/binding/service_binding_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../device/common/capability/capability_set.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../device/packages/mock_device_packages.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/host_api/host_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/host_api/service_gateway_dispatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/loader/compiled_app_contract_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/loader/llext_loader_backend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/loader/runtime_loader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/loader/stub_llext_adapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/loader/stub_loader_backend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../runtime/ownership/resource_ownership_table.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/logging/console_logging_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/audio/zephyr_audio_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/gps/zephyr_gps_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/hostlink/zephyr_hostlink_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/notification/console_notification_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/notification/zephyr_notification_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/power/zephyr_power_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/radio/zephyr_radio_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/settings/in_memory_settings_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/settings/zephyr_settings_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/storage/zephyr_storage_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/text/in_memory_text_input_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/text/zephyr_text_input_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/time/zephyr_time_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/timer/in_memory_timer_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/timer/zephyr_timer_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/ui/profile_ui_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/aegis_shell.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/control/shell_controller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/control/shell_input_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/control/shell_navigation_state.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/launcher/launcher_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../services/ui/zephyr_ui_service.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../apps/builtin/settings/settings_app_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/settings/settings_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/status/status_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../shell/status/notification_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/zephyr_llext_adapter.cpp)

if(AEGIS_ZEPHYR_BOARD_FAMILY STREQUAL "tlora_pager")
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/boards/tlora_pager/tlora_pager_board_providers.cpp
        ${CMAKE_CURRENT_LIST_DIR}/boards/tlora_pager/tlora_pager_board_runtime.cpp
        ${CMAKE_CURRENT_LIST_DIR}/boards/tlora_pager/tlora_pager_shell_display_backend.cpp
        ${CMAKE_CURRENT_LIST_DIR}/boards/tlora_pager/tlora_pager_shell_input_backend.cpp)
elseif(AEGIS_ZEPHYR_BOARD_FAMILY STREQUAL "tdeck")
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/boards/tdeck/tdeck_board_providers.cpp
        ${CMAKE_CURRENT_LIST_DIR}/boards/tdeck/tdeck_board_runtime.cpp
        ${CMAKE_CURRENT_LIST_DIR}/boards/tdeck/tdeck_shell_display_backend.cpp
        ${CMAKE_CURRENT_LIST_DIR}/boards/tdeck/tdeck_shell_input_backend.cpp)
endif()

if(AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK)
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_background/demo_background_app.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_background/demo_background_contract.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hello/demo_hello_app.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hello/demo_hello_contract.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hostlink/demo_hostlink_app.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hostlink/demo_hostlink_contract.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_info/demo_info_app.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_info/demo_info_contract.cpp)
endif()

if(AEGIS_ZEPHYR_ENABLE_APP_MODULES)
    include(${CMAKE_CURRENT_LIST_DIR}/app_modules.cmake)

    aegis_register_zephyr_app_module(
        APP_ID demo_background
        SOURCE ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_background/demo_background_app.cpp
        MANIFEST ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_background/manifest.json)
    aegis_register_zephyr_app_module(
        APP_ID demo_hello
        SOURCE ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hello/demo_hello_app.cpp
        MANIFEST ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hello/manifest.json)
    aegis_register_zephyr_app_module(
        APP_ID demo_hostlink
        SOURCE ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hostlink/demo_hostlink_app.cpp
        MANIFEST ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_hostlink/manifest.json)
    aegis_register_zephyr_app_module(
        APP_ID demo_info
        SOURCE ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_info/demo_info_app.cpp
        MANIFEST ${CMAKE_CURRENT_LIST_DIR}/../../apps/demo_info/manifest.json)
endif()
