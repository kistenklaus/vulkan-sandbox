
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(Vulkan REQUIRED)

set(GPU_LIB_NAME ${PROJECT_NAME}-gpu)

add_library(${GPU_LIB_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/buffer/Buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/buffer/Buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/cmd/CommandBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/cmd/CommandBuffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/cmd/CommandPool.fwd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/cmd/CommandPool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/ext/debug_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/ext/debug_utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/memory/DeviceMemoryResource.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/memory/DeviceMemoryResource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/memory/MonotonicResource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/memory/PolymorphicAllocator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/memory/Ptr.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/sync/Fence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/sync/Fence.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/sync/Semaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/sync/Semaphore.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/time/Timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/time/Timer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Runtime.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Runtime.fwd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Runtime.h
)

target_include_directories(${GPU_LIB_NAME} 
  PUBLIC 
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_compile_definitions(${GPU_LIB_NAME} PUBLIC 
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0 
  SPIRV_REFLECT_HAS_VULKAN_H
)

target_link_libraries(${GPU_LIB_NAME} 
  PUBLIC 
    ${Vulkan_LIBRARIES}
    wrs-logging
    wrs-spirv
    wrs-io

)
set_target_properties(${GPU_LIB_NAME}
  PROPERTIES CXX_STANDARD_REQUIRED ON
)
if( WIN32 )
  target_compile_definitions( ${GPU_LIB_NAME} PUBLIC 
    VK_USE_PLATFORM_WIN32_KHR 
  )
elseif( APPLE )
  target_compile_definitions( ${GPU_LIB_NAME} PUBLIC VK_USE_PLATFORM_METAL_EXT )
elseif( UNIX )
  target_compile_definitions( ${GPU_LIB_NAME} PUBLIC 
    VK_USE_PLATFORM_XCB_KHR 
  )
else()
  message( FATAL_ERROR, "Vulkan-Hpp: unhandled platform!" )
endif()
