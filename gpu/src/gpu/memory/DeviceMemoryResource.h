#pragma once

#include "gpu/Runtime.fwd.h"
#include "gpu/memory/Resource.h"
#include <vulkan/vulkan_handles.hpp>

namespace gpu::memory {

class DeviceMemoryResource : public Resource {
public:
  friend Runtime;

  Ptr alloc(size_t size, size_t alignment) override;
  void free(Ptr ptr) override;

  uint32_t memoryTypeIndex() const override {
    return m_memoryTypeIndex;
  }

private:
  DeviceMemoryResource(vk::Device device, uint32_t memoryTypeIndex);

  vk::Device m_device;
  uint32_t m_memoryTypeIndex;
};

} // namespace gpu::memory
