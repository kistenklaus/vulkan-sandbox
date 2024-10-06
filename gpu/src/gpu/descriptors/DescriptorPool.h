#pragma once

#include "gpu/descriptors/DescriptorSet.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace gpu {

class DescriptorPool {
public:
  DescriptorPool(vk::Device device, vk::DescriptorPool pool)
      : m_pool(pool), m_device(device) {}
  ~DescriptorPool() {
    if (m_pool) {
      m_device.destroyDescriptorPool(m_pool);
    }
  }
  DescriptorPool(const DescriptorPool &) = delete;
  DescriptorPool &operator=(const DescriptorPool &) = delete;
  DescriptorPool(DescriptorPool &&o) noexcept
      : m_pool(o.m_pool), m_device(o.m_device) {
    o.m_pool = VK_NULL_HANDLE;
    o.m_device = VK_NULL_HANDLE;
  }
  DescriptorPool &operator=(DescriptorPool &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~DescriptorPool();
    std::swap(m_pool, o.m_pool);
    std::swap(m_device, o.m_device);
    return *this;
  }

  DescriptorSet alloc(uint32_t set) {
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setSetLayouts(const vk::ArrayProxyNoTemporaries<const vk::DescriptorSetLayout> &setLayouts_)
  }

private:
  vk::DescriptorPool m_pool;
  vk::Device m_device;
};

} // namespace gpu
