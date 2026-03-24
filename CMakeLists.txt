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
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp")

add_executable(aegis
    main.cpp
    ${AEGIS_SOURCES})

target_include_directories(aegis PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include")
