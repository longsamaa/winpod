cmake_minimum_required(VERSION 3.25)

# vcpkg (submodule external/vcpkg, manifest vcpkg.json). Phải đặt TRƯỚC project(). Truyền
# -DCMAKE_TOOLCHAIN_FILE=... khi configure thì dùng cái đó thay thế.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain")
endif()

project(WinPod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generator Visual Studio là multi-config: tự thêm thư mục con Debug/ hoặc Release/ theo --config.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/lib")

# Dependencies from vcpkg.json (manifest mode, toolchain set above).
find_package(spdlog CONFIG REQUIRED)
find_package(Trantor CONFIG REQUIRED)   # trantor::EventLoop
find_package(nlohmann_json CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h" REQUIRED)   # stb là header-only, không có CMake target

# Dear ImGui từ submodule external/imgui (không có CMake riêng): build thành static
# lib cùng backend Win32 + DX11.
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
# Tự lấy mọi file trong src/. CONFIGURE_DEPENDS: mỗi lần build CMake quét lại
# thư mục, thêm/xoá file không cần chạy lại configure bằng tay.
file(GLOB_RECURSE WINPOD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
# Mọi thứ trừ main.cpp vào thư viện winpod_core, để exe và test dùng chung (biên dịch một lần).
set(WINPOD_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
list(REMOVE_ITEM WINPOD_SOURCES ${WINPOD_MAIN})

add_library(winpod_core STATIC ${WINPOD_SOURCES})

# Giữ cấu trúc thư mục src/ trong Solution Explorer của Visual Studio.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${WINPOD_SOURCES} ${WINPOD_MAIN})

# Include theo đường dẫn từ src/: #include "app/Application.hpp"
target_include_directories(winpod_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(winpod_core SYSTEM PUBLIC ${STB_INCLUDE_DIRS})

# C++/WinRT (Windows.Devices.Bluetooth, ...): RoInitialize, RoOriginateLanguageException, ...
target_link_libraries(winpod_core PUBLIC
    WindowsApp
    spdlog::spdlog
    Trantor::Trantor
    nlohmann_json::nlohmann_json
    imgui::imgui)

# WIN32: subsystem GUI, entry point wWinMain, không mở cửa sổ console. Log xem trong logs/app.log
# (hoặc stdout khi tiến trình cha có đưa stdout, vd codelldb).
add_executable(winpod WIN32 ${WINPOD_MAIN})
target_link_libraries(winpod PRIVATE winpod_core)

# main() chuyển thư mục làm việc về thư mục exe, nên resource phải nằm cạnh exe.
# Chỉ copy sprite sheet đã tách nền, không copy video gốc.
add_custom_command(TARGET winpod POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:winpod>/resources
    COMMENT "Copy resources/ (buds/, case/ sprite sheets, fonts/)")

# windows.h: bỏ phần ít dùng (và winsock.h cũ, tránh xung đột với winsock2.h của
# trantor); tắt macro min/max để không phá std::min/std::max. Nhớ thêm -D tương
# ứng vào compile_flags.txt cho clangd.
# PUBLIC: exe và test include cùng header nên phải thấy cùng các define này.
target_compile_definitions(winpod_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)

# Mọi header Windows SDK / C++/WinRT gom trong platform/WindowsSdk.hpp; biên dịch sẵn một lần
# (precompiled header) thay vì mỗi file .cpp phải parse lại windows.h + winrt.
target_precompile_headers(winpod_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/WindowsSdk.hpp)

foreach(target winpod_core winpod)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

# Unit test (GoogleTest): build/bin/<Config>/winpod_tests.exe, chạy bằng `ctest --test-dir build -C Debug`.
option(WINPOD_BUILD_TESTS "Build unit tests" ON)
if(WINPOD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
