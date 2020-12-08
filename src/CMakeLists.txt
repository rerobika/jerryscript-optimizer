set(SRC
    basic-block.cpp
    inst.cpp
)

add_library(optimizer-core STATIC ${SRC})

target_include_directories (optimizer-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
