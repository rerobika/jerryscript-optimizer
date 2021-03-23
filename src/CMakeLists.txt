# Copyright (c) 2020 Robert Fancsik
#
# Licensed under the BSD 3-Clause License
# <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
# This file may not be copied, modified, or distributed except
# according to those terms.

set(SRC
    basic-block.cpp
    bytecode.cpp
    dominator.cpp
    inst.cpp
    ir-builder.cpp
    liveness-analyzer.cpp
    optimizer.cpp
    pass.cpp
    register-compresser.cpp
    snapshot-readwriter.cpp
    stack.cpp
    value.cpp
)

add_library(optimizer-core STATIC ${SRC})
add_dependencies(optimizer-core jerry-core)

message(${CMAKE_BINARY_DIR})
target_include_directories (optimizer-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_BINARY_DIR}/deps/jerryscript/amalgam/
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/api
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/include
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/jrt
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/lit
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/jmem
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/ecma/base
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/ecma/operations
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/ecma/builtin-objects
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/vm
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-core/parser/js
  PUBLIC ${JERRYSCRIPT_DIR}/jerry-port/default/include
)
target_link_libraries (optimizer-core
  PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libjerry-core${CMAKE_STATIC_LIBRARY_SUFFIX}
  PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libjerry-port-default${CMAKE_STATIC_LIBRARY_SUFFIX})
