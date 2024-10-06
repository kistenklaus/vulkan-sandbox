set(LOGGING_LIB_NAME wrs-logging)

add_library(${LOGGING_LIB_NAME} 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/logging.h
)

target_include_directories(${LOGGING_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(${LOGGING_LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
