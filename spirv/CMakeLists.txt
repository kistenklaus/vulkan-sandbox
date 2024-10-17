
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(Vulkan REQUIRED)

set(SPIRV_LIB_NAME wrs-spirv)


add_library(${SPIRV_LIB_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spirv/compile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spirv/compile.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spirv/reflect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spirv/reflect.h
)

target_include_directories(${SPIRV_LIB_NAME} 
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_compile_definitions(${SPIRV_LIB_NAME} PUBLIC 
  SPIRV_REFLECT_HAS_VULKAN_H
)

target_link_libraries(${SPIRV_LIB_NAME} 
  PRIVATE 
    shaderc_shared
    wrs-logging
)
set_target_properties(${SPIRV_LIB_NAME}
  PROPERTIES CXX_STANDARD_REQUIRED ON
)
