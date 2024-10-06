#include "./Buffer.h"
#include <cstdlib>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include "gpu/Runtime.h"

using namespace gpu;

Buffer::Buffer(const Runtime &runtime, size_t size, vk::BufferUsageFlags usage)
    : m_device(runtime.device) {

  vk::BufferCreateInfo createInfo;

  vk::Result rCreateBuffer =
      m_device.createBuffer(&createInfo, nullptr, &m_buffer);
  if (rCreateBuffer != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create buffer");
  }

  vk::MemoryAllocateInfo allocInfo;
  vk::Result rAllocMemory =
      m_device.allocateMemory(&allocInfo, nullptr, &m_deviceMemory);
  if (rAllocMemory != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to allocate memory");
  }
}
Buffer::~Buffer() {
  m_device.destroyBuffer(m_buffer);
  m_device.freeMemory(m_deviceMemory);
}
Buffer::Buffer(Buffer &&o) noexcept
    : m_buffer(o.m_buffer), m_deviceMemory(o.m_deviceMemory),
      m_device(o.m_device) {
  o.m_buffer = VK_NULL_HANDLE;
  o.m_deviceMemory = VK_NULL_HANDLE;
  o.m_device = VK_NULL_HANDLE;
}
Buffer &Buffer::operator=(Buffer &&o) noexcept {
  if (this == &o) {
    return *this;
  }
  this->~Buffer();
  std::swap(m_buffer, o.m_buffer);
  std::swap(m_deviceMemory, o.m_deviceMemory);
  std::swap(m_device, o.m_device);
  return *this;
}
