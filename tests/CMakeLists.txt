find_package(Catch2 QUIET)

if(NOT Catch2_FOUND)
    add_library(catch2_fallback_main STATIC
        ${PROJECT_SOURCE_DIR}/third_party/catch2_fallback/catch_main.cpp
    )
    target_include_directories(catch2_fallback_main PUBLIC
        ${PROJECT_SOURCE_DIR}/third_party/catch2_fallback
    )
    add_library(Catch2::Catch2WithMain ALIAS catch2_fallback_main)
endif()

add_executable(integer_tests test_integer.cpp)
target_link_libraries(integer_tests PRIVATE integer md5 Catch2::Catch2WithMain)
auto_detect_compiler_warnings(integer_tests PRIVATE)
auto_detect_sanitizers(integer_tests)

if(Catch2_FOUND)
    include(Catch)
    catch_discover_tests(integer_tests)
else()
    add_test(NAME integer_tests COMMAND integer_tests)
endif()
