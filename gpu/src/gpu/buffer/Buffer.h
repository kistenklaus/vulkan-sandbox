#pragma once

#include "gpu/Runtime.h"
#include "gpu/memory/PolymorphicAllocator.h"
#include <cassert>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace gpu {

template <typename GpuAllocator = memory::PolymorphicAllocator> class Buffer {
  friend class DescriptorSet;

public:
  Buffer(const Runtime &runtime, size_t size, vk::BufferUsageFlags usage,
         const GpuAllocator &allocator)
      : m_device(runtime.device), m_gpuAllocator(allocator) {
    vk::BufferCreateInfo createInfo;
    createInfo.setUsage(usage);
    createInfo.setSize(static_cast<vk::DeviceSize>(size));
    createInfo.setSharingMode(vk::SharingMode::eExclusive);
    createInfo.setQueueFamilyIndices(runtime.graphicsQueueFamilyIndex);

    vk::Result rCreateBuffer =
        m_device.createBuffer(&createInfo, nullptr, &m_buffer);
    if (rCreateBuffer != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create buffer");
    }
    vk::MemoryRequirements memoryRequirements =
        m_device.getBufferMemoryRequirements(m_buffer);
    assert(memoryRequirements.memoryTypeBits &
           (1 << m_gpuAllocator.memoryTypeIndex()));
    m_memory = m_gpuAllocator.alloc(memoryRequirements.size,
                                    memoryRequirements.alignment);
    m_device.bindBufferMemory(m_buffer, m_memory.deviceMemory, m_memory.offset);
  }
  ~Buffer() {
    m_gpuAllocator.free(m_memory);
    m_device.destroyBuffer(m_buffer);
  }
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  Buffer(Buffer &&) noexcept;
  Buffer &operator=(Buffer &&) noexcept;

private:
  vk::Device m_device;
  [[no_unique_address]] GpuAllocator m_gpuAllocator;
  vk::Buffer m_buffer;
  memory::Ptr m_memory;
};

namespace pmr {
using Buffer = Buffer<memory::PolymorphicAllocator>;
}

} // namespace gpu
