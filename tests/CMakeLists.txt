find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE WINPOD_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)
add_executable(winpod_tests ${WINPOD_TEST_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${WINPOD_TEST_SOURCES})
target_include_directories(winpod_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(winpod_tests PRIVATE winpod_core GTest::gtest GTest::gtest_main)
target_compile_definitions(winpod_tests PRIVATE WINPOD_RESOURCES_DIR="${PROJECT_SOURCE_DIR}/resources")

if(MSVC)
    target_compile_options(winpod_tests PRIVATE /W4 /permissive- /utf-8)
endif()

include(GoogleTest)
gtest_discover_tests(winpod_tests DISCOVERY_MODE PRE_TEST)
