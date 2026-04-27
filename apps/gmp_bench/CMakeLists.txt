list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(GMP QUIET)

if(NOT GMP_FOUND)
    message(STATUS "GMP not found; skipping gmp_bench")
    return()
endif()

add_executable(gmp_bench main.cpp)
target_link_libraries(gmp_bench PRIVATE gmp gmpxx)
