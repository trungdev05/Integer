find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    add_library(benchmark_fallback INTERFACE)
    target_include_directories(benchmark_fallback INTERFACE
        ${PROJECT_SOURCE_DIR}/third_party/benchmark_fallback
    )
    add_library(benchmark::benchmark ALIAS benchmark_fallback)
endif()

add_executable(integer_benchmark bench_integer.cpp)
target_link_libraries(integer_benchmark PRIVATE integer md5 benchmark::benchmark)
auto_detect_compiler_warnings(integer_benchmark PRIVATE)
auto_detect_sanitizers(integer_benchmark)
