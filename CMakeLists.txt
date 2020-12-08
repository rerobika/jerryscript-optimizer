cmake_minimum_required (VERSION 3.5.0)
project (jerryscript-optimizer)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Werror -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DEPS_DIR "${PROJECT_SOURCE_DIR}/deps")
set(ARG_PARSE_DIR "${DEPS_DIR}/argparse")
set(JERRYSCRIPT_DIR "${DEPS_DIR}/jerryscript")

add_subdirectory(src)
add_subdirectory(main)
