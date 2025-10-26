include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)

FetchContent_Declare(
    GoogleBenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(GoogleBenchmark)

add_executable(integer_benchmark bench_integer.cpp)
target_link_libraries(integer_benchmark PRIVATE integer md5 benchmark::benchmark)
auto_detect_compiler_warnings(integer_benchmark PRIVATE)
auto_detect_sanitizers(integer_benchmark)
