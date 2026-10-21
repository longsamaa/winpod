cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain")
endif()

project(WinPod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/lib")

find_package(spdlog CONFIG REQUIRED)
find_package(Trantor CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h" REQUIRED)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)
add_library(imgui::imgui ALIAS imgui)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler dwmapi)

file(GLOB_RECURSE WINPOD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
set(WINPOD_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
list(REMOVE_ITEM WINPOD_SOURCES ${WINPOD_MAIN})

add_library(winpod_core STATIC ${WINPOD_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${WINPOD_SOURCES} ${WINPOD_MAIN})

target_include_directories(winpod_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(winpod_core SYSTEM PUBLIC ${STB_INCLUDE_DIRS})

target_link_libraries(winpod_core PUBLIC
    WindowsApp
    spdlog::spdlog
    Trantor::Trantor
    nlohmann_json::nlohmann_json
    imgui::imgui)

add_executable(winpod WIN32 ${WINPOD_MAIN})
target_link_libraries(winpod PRIVATE winpod_core)

add_custom_command(TARGET winpod POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:winpod>/resources
    COMMENT "Copy resources/ (buds/, case/ sprite sheets, fonts/)")

target_compile_definitions(winpod_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)

target_precompile_headers(winpod_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/WindowsSdk.hpp)

foreach(target winpod_core winpod)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

option(WINPOD_BUILD_TESTS "Build unit tests" ON)
if(WINPOD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
