cmake_minimum_required(VERSION 3.20)

project(aegis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE AEGIS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/services/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/device/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shell/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ports/host/*.cpp")

list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_settings_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_notification_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_power_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_radio_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_gps_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_storage_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_time_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_timer_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_audio_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_hostlink_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_text_input_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/zephyr_ui_service\\.cpp$")
list(FILTER AEGIS_SOURCES EXCLUDE REGEX ".*/generated/zephyr/.*\\.cpp$")

add_executable(aegis_host_sim ${AEGIS_SOURCES})

target_sources(aegis_host_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/handles/app_handle_table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/quota/quota_ledger.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/supervisor/app_instance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/supervisor/app_supervisor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/syscall/syscall_gateway.cpp")

target_include_directories(aegis_host_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include")
