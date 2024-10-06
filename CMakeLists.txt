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

add_subdirectory(sandbox)

