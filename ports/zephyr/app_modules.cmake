include(CMakeParseArguments)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT DEFINED AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR)
    set(AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR "${CMAKE_BINARY_DIR}/aegis/apps")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APP_DEPLOY_ROOT)
    set(AEGIS_ZEPHYR_APP_DEPLOY_ROOT "${CMAKE_BINARY_DIR}/deploy/lfs/apps")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_IMAGE_OUTPUT)
    set(AEGIS_ZEPHYR_APPFS_IMAGE_OUTPUT "${CMAKE_BINARY_DIR}/deploy/appfs.bin")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_LAYOUT_JSON)
    set(AEGIS_ZEPHYR_APPFS_LAYOUT_JSON "${CMAKE_BINARY_DIR}/deploy/appfs-layout.json")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_BLOCK_SIZE)
    set(AEGIS_ZEPHYR_APPFS_BLOCK_SIZE "4096")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_PAGE_SIZE)
    set(AEGIS_ZEPHYR_APPFS_PAGE_SIZE "256")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_IMAGE_TOOL)
    set(AEGIS_ZEPHYR_APPFS_IMAGE_TOOL "")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_TOOL_CACHE_DIR)
    set(AEGIS_ZEPHYR_APPFS_TOOL_CACHE_DIR "${CMAKE_BINARY_DIR}/tools/mklittlefs")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_APPFS_AUTO_FETCH_TOOL)
    set(AEGIS_ZEPHYR_APPFS_AUTO_FETCH_TOOL ON)
endif()
if(NOT DEFINED AEGIS_ZEPHYR_FLASH_TOOL)
    set(AEGIS_ZEPHYR_FLASH_TOOL "")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_FLASH_CHIP)
    set(AEGIS_ZEPHYR_FLASH_CHIP "esp32s3")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_FLASH_BAUD)
    set(AEGIS_ZEPHYR_FLASH_BAUD "921600")
endif()
if(NOT DEFINED AEGIS_ZEPHYR_FLASH_PORT)
    set(AEGIS_ZEPHYR_FLASH_PORT "")
endif()

add_custom_target(aegis_zephyr_prepare_app_modules
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR}"
    COMMENT "Preparing clean Zephyr app module staging root at ${AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR}")
add_custom_target(aegis_zephyr_app_modules)
add_dependencies(aegis_zephyr_app_modules aegis_zephyr_prepare_app_modules)
add_custom_target(aegis_zephyr_deploy_app_tree
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${AEGIS_ZEPHYR_APP_DEPLOY_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${AEGIS_ZEPHYR_APP_DEPLOY_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR}"
        "${AEGIS_ZEPHYR_APP_DEPLOY_ROOT}"
    DEPENDS aegis_zephyr_app_modules
    COMMENT "Assembling deployable Zephyr app package tree under ${AEGIS_ZEPHYR_APP_DEPLOY_ROOT}")
add_custom_target(aegis_zephyr_appfs_image
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_LIST_DIR}/scripts/build_appfs_image.py"
        --zephyr-dts "${CMAKE_BINARY_DIR}/zephyr/zephyr.dts"
        --deploy-root "${AEGIS_ZEPHYR_APP_DEPLOY_ROOT}"
        --output "${AEGIS_ZEPHYR_APPFS_IMAGE_OUTPUT}"
        --layout-json "${AEGIS_ZEPHYR_APPFS_LAYOUT_JSON}"
        "--mklittlefs=${AEGIS_ZEPHYR_APPFS_IMAGE_TOOL}"
        "--tool-cache-dir=${AEGIS_ZEPHYR_APPFS_TOOL_CACHE_DIR}"
        $<$<BOOL:${AEGIS_ZEPHYR_APPFS_AUTO_FETCH_TOOL}>:--allow-auto-fetch>
        --block-size "${AEGIS_ZEPHYR_APPFS_BLOCK_SIZE}"
        --page-size "${AEGIS_ZEPHYR_APPFS_PAGE_SIZE}"
    DEPENDS aegis_zephyr_deploy_app_tree
    COMMENT "Building board-flashable Aegis appfs image at ${AEGIS_ZEPHYR_APPFS_IMAGE_OUTPUT}")
add_custom_target(aegis_zephyr_flash_appfs
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_LIST_DIR}/scripts/flash_appfs_image.py"
        --zephyr-dts "${CMAKE_BINARY_DIR}/zephyr/zephyr.dts"
        --image "${AEGIS_ZEPHYR_APPFS_IMAGE_OUTPUT}"
        "--port=${AEGIS_ZEPHYR_FLASH_PORT}"
        "--baud=${AEGIS_ZEPHYR_FLASH_BAUD}"
        "--chip=${AEGIS_ZEPHYR_FLASH_CHIP}"
        "--flash-tool=${AEGIS_ZEPHYR_FLASH_TOOL}"
    DEPENDS aegis_zephyr_appfs_image
    COMMENT "Flashing Aegis appfs image into storage_partition")

function(aegis_register_zephyr_app_modules_from_directory)
    set(options INCLUDE_EXAMPLE_APPS)
    set(oneValueArgs BASE_DIR)
    cmake_parse_arguments(AEGIS_SCAN "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT AEGIS_SCAN_BASE_DIR)
        message(FATAL_ERROR "aegis_register_zephyr_app_modules_from_directory requires BASE_DIR")
    endif()
    if(NOT IS_DIRECTORY "${AEGIS_SCAN_BASE_DIR}")
        message(FATAL_ERROR "app module base directory not found: ${AEGIS_SCAN_BASE_DIR}")
    endif()

    file(GLOB package_manifests CONFIGURE_DEPENDS
        "${AEGIS_SCAN_BASE_DIR}/*/manifest.json")

    foreach(manifest_path IN LISTS package_manifests)
        get_filename_component(package_root "${manifest_path}" DIRECTORY)
        get_filename_component(app_id "${package_root}" NAME)

        if(app_id STREQUAL "builtin")
            continue()
        endif()
        if(app_id MATCHES "^demo_" AND NOT AEGIS_SCAN_INCLUDE_EXAMPLE_APPS)
            continue()
        endif()

        file(GLOB app_sources CONFIGURE_DEPENDS "${package_root}/*_app.cpp")
        list(LENGTH app_sources app_source_count)
        if(app_source_count EQUAL 0)
            message(WARNING "Skipping app package ${app_id}: no *_app.cpp found in ${package_root}")
            continue()
        endif()
        if(NOT app_source_count EQUAL 1)
            message(WARNING "Skipping app package ${app_id}: expected exactly one *_app.cpp in ${package_root}")
            continue()
        endif()

        set(icon_path "${package_root}/icon.bin")
        if(NOT EXISTS "${icon_path}")
            set(icon_path "")
        endif()

        list(GET app_sources 0 app_source)
        aegis_register_zephyr_app_module(
            APP_ID "${app_id}"
            SOURCE "${app_source}"
            MANIFEST "${manifest_path}"
            PACKAGE_ROOT "${package_root}"
            ICON "${icon_path}")
    endforeach()
endfunction()

function(aegis_register_zephyr_app_module)
    set(options)
    set(oneValueArgs APP_ID SOURCE MANIFEST ICON PACKAGE_ROOT)
    cmake_parse_arguments(AEGIS "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT AEGIS_APP_ID)
        message(FATAL_ERROR "aegis_register_zephyr_app_module requires APP_ID")
    endif()
    if(NOT AEGIS_SOURCE)
        message(FATAL_ERROR "aegis_register_zephyr_app_module requires SOURCE")
    endif()
    if(NOT EXISTS "${AEGIS_SOURCE}")
        message(FATAL_ERROR "app module source not found: ${AEGIS_SOURCE}")
    endif()
    if(NOT AEGIS_MANIFEST OR NOT EXISTS "${AEGIS_MANIFEST}")
        message(FATAL_ERROR "app module manifest not found: ${AEGIS_MANIFEST}")
    endif()
    if(NOT AEGIS_PACKAGE_ROOT)
        get_filename_component(AEGIS_PACKAGE_ROOT "${AEGIS_MANIFEST}" DIRECTORY)
    endif()
    if(NOT IS_DIRECTORY "${AEGIS_PACKAGE_ROOT}")
        message(FATAL_ERROR "app module package root not found: ${AEGIS_PACKAGE_ROOT}")
    endif()

    set(module_target "${AEGIS_APP_ID}_llext_module")
    set(module_library_target "${module_target}_llext_lib")
    set(module_output_dir "${AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR}/${AEGIS_APP_ID}")
    set(module_output_file "${module_output_dir}/app.llext")
    set(module_debug_elf "${CMAKE_BINARY_DIR}/llext/${module_target}_debug.elf")
    set(module_input_object "$<TARGET_OBJECTS:${module_library_target}>")
    set(module_assets_dir "${AEGIS_PACKAGE_ROOT}/assets")

    file(MAKE_DIRECTORY "${module_output_dir}")

    add_llext_target(${module_target}
        OUTPUT "${module_output_file}"
        SOURCES "${AEGIS_SOURCE}")

    llext_compile_features(${module_target} cxx_std_20)
    target_compile_options(${module_library_target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-non-call-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-unwind-tables>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-asynchronous-unwind-tables>)
    llext_include_directories(${module_target}
        ${CMAKE_CURRENT_LIST_DIR}/../..
        ${CMAKE_CURRENT_LIST_DIR}/../../sdk/include)

    set(module_stage_target "${module_target}_stage")
    set(module_stage_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${module_output_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${module_input_object}"
            "${module_debug_elf}"
        COMMAND ${CMAKE_OBJCOPY}
            --strip-debug
            --remove-section=.xt.*
            --remove-section=.xtensa.info
            "${module_debug_elf}"
            "${module_output_file}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AEGIS_MANIFEST}"
            "${module_output_dir}/manifest.json")

    if(AEGIS_ICON AND EXISTS "${AEGIS_ICON}")
        list(APPEND module_stage_commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${AEGIS_ICON}"
                "${module_output_dir}/icon.bin")
    endif()

    if(EXISTS "${module_assets_dir}")
        list(APPEND module_stage_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${module_assets_dir}"
                "${module_output_dir}/assets")
    endif()

    add_custom_target(${module_stage_target}
        ${module_stage_commands}
        DEPENDS aegis_zephyr_prepare_app_modules ${module_library_target}
        COMMENT "Staging Zephyr app module ${AEGIS_APP_ID}"
        VERBATIM)

    add_dependencies(aegis_zephyr_app_modules ${module_stage_target})
endfunction()
