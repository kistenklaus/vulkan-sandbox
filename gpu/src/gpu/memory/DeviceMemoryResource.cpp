#include "./DeviceMemoryResource.h"
#include "gpu/Runtime.h"

gpu::memory::DeviceMemoryResource::DeviceMemoryResource(
    vk::Device device, uint32_t memoryTypeIndex)
    : m_device(device), m_memoryTypeIndex(memoryTypeIndex) {}

gpu::memory::Ptr gpu::memory::DeviceMemoryResource::alloc(size_t size,
                                                          size_t alignment) {
  vk::MemoryAllocateInfo allocInfo;
  allocInfo.setMemoryTypeIndex(m_memoryTypeIndex);
  allocInfo.setAllocationSize(size);
  Ptr ptr;
  ptr.offset = 0;
  vk::Result rAllocateMemory =
      m_device.allocateMemory(&allocInfo, nullptr, &ptr.deviceMemory);
  if (rAllocateMemory != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to allocate memory");
  }
  return ptr;
}

void gpu::memory::DeviceMemoryResource::free(Ptr ptr) {
  m_device.freeMemory(ptr.deviceMemory);
}
