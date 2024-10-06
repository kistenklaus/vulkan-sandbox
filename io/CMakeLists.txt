

set(IO_LIB_NAME wrs-io)

add_library(${IO_LIB_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/io.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/io.h
)

target_include_directories(${IO_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
