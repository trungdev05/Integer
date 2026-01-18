list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(GMP REQUIRED)
add_executable(gmp_bench main.cpp)
target_link_libraries(gmp_bench PRIVATE gmp gmpxx)
