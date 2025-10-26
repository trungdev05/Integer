cmake_minimum_required(VERSION 3.20)
project(Integer LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INTEGER_ENABLE_WARNINGS "Enable warning flags" ON)
option(INTEGER_ENABLE_SANITIZERS "Enable address sanitizers in debug builds" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerSettings)

add_library(integer INTERFACE)
target_include_directories(integer INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)
target_compile_features(integer INTERFACE cxx_std_20)

add_library(md5 STATIC src/md5.cpp)
target_include_directories(md5 PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_compile_features(md5 PUBLIC cxx_std_20)

auto_detect_compiler_warnings(integer INTERFACE)
auto_detect_compiler_warnings(md5 PRIVATE)

auto_detect_sanitizers(md5)

include(CTest)

add_subdirectory(apps)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
add_subdirectory(benchmarks)

configure_file("${PROJECT_SOURCE_DIR}/data/baseline.json"
               "${PROJECT_BINARY_DIR}/baseline.json"
               COPYONLY)

include(GNUInstallDirs)
install(FILES "${PROJECT_SOURCE_DIR}/include/integer.hpp" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS md5)
install(DIRECTORY scripts DESTINATION share/${PROJECT_NAME} PATTERN "__pycache__" EXCLUDE)
