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

add_custom_target(aegis_zephyr_app_modules)
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

function(aegis_register_zephyr_app_module)
    set(options)
    set(oneValueArgs APP_ID SOURCE MANIFEST ICON)
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

    set(module_target "${AEGIS_APP_ID}_llext_module")
    set(module_library_target "${module_target}_llext_lib")
    set(module_output_dir "${AEGIS_ZEPHYR_APP_MODULE_STAGING_DIR}/${AEGIS_APP_ID}")
    set(module_output_file "${module_output_dir}/app.llext")
    set(module_debug_elf "${CMAKE_BINARY_DIR}/llext/${module_target}_debug.elf")

    file(MAKE_DIRECTORY "${module_output_dir}")

    add_llext_target(${module_target}
        OUTPUT "${module_output_file}"
        SOURCES "${AEGIS_SOURCE}")

    llext_compile_features(${module_target} cxx_std_20)
    llext_include_directories(${module_target}
        ${CMAKE_CURRENT_LIST_DIR}/../..
        ${CMAKE_CURRENT_LIST_DIR}/../../sdk/include)

    set(module_stage_target "${module_target}_stage")
    set(module_stage_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${module_output_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${module_library_target}>"
            "${module_debug_elf}"
        COMMAND ${CMAKE_OBJCOPY}
            --strip-unneeded
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

    add_custom_target(${module_stage_target}
        ${module_stage_commands}
        DEPENDS ${module_library_target}
        COMMENT "Staging Zephyr app module ${AEGIS_APP_ID}"
        VERBATIM)

    add_dependencies(aegis_zephyr_app_modules ${module_stage_target})
endfunction()
