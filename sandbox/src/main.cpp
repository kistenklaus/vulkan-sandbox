#include "gpu/buffer/Buffer.h"
#include "gpu/cmd/CommandPool.h"
#include "gpu/memory/MonotonicResource.h"
#include <gpu/buffer/Buffer.h>
#include <gpu/gpu.h>
#include <gpu/memory/memory.h>
#include <logging.h>
#include <memory_resource>
#include <vulkan/vulkan_enums.hpp>

int main() {

  gpu::Runtime runtime;
  {

    gpu::memory::MonotonicResource gpuBufferResource{runtime.deviceLocalMemory(),
                                                  4096};

    std::pmr::monotonic_buffer_resource cpuBufferResouce {4096};

    gpu::pmr::Buffer buffer(runtime, 1000,
                            vk::BufferUsageFlagBits::eStorageBuffer,
                            &gpuBufferResource);

    gpu::pmr::Buffer buffer2(runtime, 1000,
                            vk::BufferUsageFlagBits::eStorageBuffer,
                            &gpuBufferResource);


    /* gpu::ComputePipeline computePipeline1(runtime, "./shaders/basic.cs.glsl"); */
    /* gpu::ComputePipeline computePipeline2(runtime, "./shaders/basic.cs.glsl"); */
    /*  */
    /*  */
    /* gpu::DescriptorPool pool (computePipeline.description()  , 10); */
    
  }
}
