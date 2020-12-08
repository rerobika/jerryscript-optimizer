include_directories (${ARG_PARSE_DIR})
add_executable (jerryscript-optimizer "main.cpp")

target_link_libraries (jerryscript-optimizer LINK_PUBLIC optimizer-core)
