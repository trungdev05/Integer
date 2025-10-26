include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(integer_tests test_integer.cpp)
target_link_libraries(integer_tests PRIVATE integer md5 Catch2::Catch2WithMain)
auto_detect_compiler_warnings(integer_tests PRIVATE)
auto_detect_sanitizers(integer_tests)

include(Catch)
catch_discover_tests(integer_tests)
