cmake_minimum_required(VERSION 3.22)

project(OutlineRGB LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    outlinergb
    SHARED
    src/main.cpp
    src/hook.cpp
    src/renderer.cpp
    src/resolver.cpp
)

target_include_directories(
    outlinergb
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(
    outlinergb
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(
    outlinergb
    log
    dl
)
