#pragma once
#include "gpu/Runtime.fwd.h"
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>

namespace gpu {

class Fence {
public:
  Fence(const Runtime &runtime, vk::FenceCreateFlags flags = {});
  ~Fence() {
    if (m_fence) {
      m_device.destroyFence(m_fence);
    }
    m_device = VK_NULL_HANDLE;
    m_fence = VK_NULL_HANDLE;
  }
  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;
  Fence(Fence &&o) noexcept
      : m_device(std::move(o.m_device)), m_fence(std::move(o.m_fence)) {
    o.m_device = VK_NULL_HANDLE;
    o.m_fence = VK_NULL_HANDLE;
  }
  Fence &operator=(Fence &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~Fence();
    std::swap(m_device, o.m_device);
    std::swap(m_fence, o.m_fence);
    return *this;
  }

  inline vk::Fence handle() const { return m_fence; }

  void wait() {
    vk::Result rWaitForFence = m_device.waitForFences(1, &m_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    if (rWaitForFence != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence");
    }
  }

  void reset() {
    vk::Result rResetFences = m_device.resetFences(1, &m_fence);
    if (rResetFences != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to reset fences");
    }
  }

private:
  vk::Device m_device;
  vk::Fence m_fence;
};
} // namespace gpu
