# Copyright (c) 2020 Robert Fancsik
#
# Licensed under the BSD 3-Clause License
# <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
# This file may not be copied, modified, or distributed except
# according to those terms.

cmake_minimum_required(VERSION 3.5)

set(JERRYSCRIPT_PREFIX deps/jerryscript)
ExternalProject_Add(libjerryscript
  PREFIX ${JERRYSCRIPT_PREFIX}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/deps/jerryscript/
  BUILD_IN_SOURCE 0
  BINARY_DIR ${JERRYSCRIPT_PREFIX}
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/${JERRYSCRIPT_PREFIX}/lib/${CONFIG_TYPE}
    ${CMAKE_BINARY_DIR}/lib/
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_BUILD_TYPE=${JERRY_CMAKE_BUILD_TYPE}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DENABLE_ALL_IN_ONE=OFF
    -DJERRY_CMDLINE=OFF
    -DJERRY_EXT=OFF
    -DJERRY_LIBM=OFF
    -DJERRY_PORT_DEFAULT=OFF
    -DJERRY_SNAPSHOT_EXEC=ON
    -DJERRY_SNAPSHOT_SAVE=ON
    -DJERRY_LOGGING=ON
    -DJERRY_LINE_INFO=ON
    -DENABLE_LTO=ON
)

set_property(DIRECTORY APPEND PROPERTY
  ADDITIONAL_MAKE_CLEAN_FILES
    ${CMAKE_BINARY_DIR}/lib/${JERRY_CORE_NAME}
)

add_library(jerry-core STATIC IMPORTED)
add_dependencies(jerry-core libjerryscript)
set_property(TARGET jerry-core PROPERTY
  IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/${JERRY_CORE_NAME})
