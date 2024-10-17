cmake_minimum_required(VERSION 3.30)
project(wrs CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_subdirectory(logging)

add_subdirectory(io)

add_subdirectory(spirv)

add_subdirectory(gpu)

add_library(${PROJECT_NAME} 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/DecoupledLookBackPrefixSum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/DecoupledLookBackPrefixSum.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SubgroupPrefixSum.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SubgroupPrefixSum.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/TrivialApp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/TrivialApp.h
)
target_link_libraries(${PROJECT_NAME} 
  PUBLIC wrs-gpu)
target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(sandbox)

add_subdirectory(bench)
