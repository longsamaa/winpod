find_package(GTest CONFIG REQUIRED)

# tests/fakes/    : bản giả của interface (FakeRenderer...)
# tests/<module>/ : test cho src/<module>/, cùng tên thư mục
file(GLOB_RECURSE WINPOD_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)
add_executable(winpod_tests ${WINPOD_TEST_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${WINPOD_TEST_SOURCES})
# #include "fakes/FakeRenderer.hpp"
target_include_directories(winpod_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(winpod_tests PRIVATE winpod_core GTest::gtest GTest::gtest_main)
# Test đọc thẳng resource trong repo, không cần bước copy sau build.
target_compile_definitions(winpod_tests PRIVATE WINPOD_RESOURCES_DIR="${PROJECT_SOURCE_DIR}/resources")

if(MSVC)
    target_compile_options(winpod_tests PRIVATE /W4 /permissive- /utf-8)
endif()

include(GoogleTest)
# Chạy exe lúc ctest (không phải lúc build) để liệt kê test: build không fail vì thiếu DLL.
gtest_discover_tests(winpod_tests DISCOVERY_MODE PRE_TEST)
