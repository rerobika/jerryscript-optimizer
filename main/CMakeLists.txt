# Copyright (c) 2020 Robert Fancsik
#
# Licensed under the BSD 3-Clause License
# <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
# This file may not be copied, modified, or distributed except
# according to those terms.

# include_directories (${ARG_PARSE_DIR})
add_executable (jerryscript-optimizer "main.cpp")

target_link_libraries (jerryscript-optimizer LINK_PUBLIC optimizer-core)

target_include_directories (jerryscript-optimizer
  PUBLIC ${ARG_PARSE_DIR}
  PUBLIC ${SRC_DIR}/include
)
