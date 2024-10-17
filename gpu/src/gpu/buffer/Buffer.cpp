#include "./Buffer.h"
#include "gpu/Runtime.h"
#include "gpu/Queue.h"


gpu::Buffer::Buffer(const Runtime &runtime, size_t size,
                    vk::BufferUsageFlags usage,
                    const Queue* queue,
                    const memory::PolymorphicAllocator &allocator)
    : m_device(runtime.device), m_gpuAllocator(allocator), m_size(size) {
  vk::BufferCreateInfo createInfo;
  createInfo.setUsage(usage);
  createInfo.setSize(static_cast<vk::DeviceSize>(size));
  createInfo.setSharingMode(vk::SharingMode::eExclusive);
  uint32_t familyIndex = queue->familyIndex();
  createInfo.setQueueFamilyIndices(familyIndex);

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

