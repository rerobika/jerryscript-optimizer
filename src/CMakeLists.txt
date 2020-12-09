# Copyright (c) 2020 Robert Fancsik
#
# Licensed under the BSD 3-Clause License
# <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
# This file may not be copied, modified, or distributed except
# according to those terms.

set(SRC
    basic-block.cpp
    bytecode.cpp
    inst.cpp
    optimizer.cpp
    snapshot-reader.cpp
)

add_library(optimizer-core STATIC ${SRC})
add_dependencies(optimizer-core jerry-core)

target_include_directories (optimizer-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PUBLIC ${JERRY_INCLUDE_DIR}
)
target_link_libraries (optimizer-core PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libjerry-core${CMAKE_STATIC_LIBRARY_SUFFIX})
