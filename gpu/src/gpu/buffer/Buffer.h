#pragma once

#include "gpu/Queue.fwd.h"
#include "gpu/Runtime.fwd.h"
#include "gpu/memory/PolymorphicAllocator.h"
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace gpu {

class Buffer {
  friend class DescriptorSet;

public:
  Buffer(const Runtime &runtime, size_t size, vk::BufferUsageFlags usage, const Queue* queue,
         const memory::PolymorphicAllocator &allocator);
  ~Buffer() {
    m_gpuAllocator.free(m_memory);
    m_device.destroyBuffer(m_buffer);
  }
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
  Buffer(Buffer &&) noexcept;
  Buffer &operator=(Buffer &&) noexcept;

  void* map(size_t offset = 0) {
    void* data;
    vk::Result rMapMemory = m_device.mapMemory(m_memory.deviceMemory, m_memory.offset + offset, m_size - offset, {}, &data);
    if (rMapMemory != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to map memory");
    }
    return data;
  }

  void unmap() {
    m_device.unmapMemory(m_memory.deviceMemory);
  }

  template<typename T = float>
  void upload(std::span<T> data, size_t offset = 0) {
    void* mappedData = map(offset);
    std::memcpy(mappedData, data.data(), data.size_bytes());
    m_device.unmapMemory(m_memory.deviceMemory);
  }

  template<typename T, typename Allocator = std::allocator<T>>
  std::vector<float> download(size_t offset = 0, const Allocator& alloc = {}) {
    std::vector<float> downstream((m_size - offset) / sizeof(float));
    void* mappedData = map(offset);
    std::memcpy(downstream.data(), mappedData, m_size - offset);
    m_device.unmapMemory(m_memory.deviceMemory);
    return std::move(downstream);
  }

  inline vk::Buffer handle() const {
    return m_buffer;
  }

  inline vk::DeviceSize size() const {
    return static_cast<vk::DeviceSize>(m_size);
  }

private:
  vk::Device m_device;
  memory::PolymorphicAllocator m_gpuAllocator;
  vk::Buffer m_buffer;
  memory::Ptr m_memory;
  size_t m_size;
};

} // namespace gpu
