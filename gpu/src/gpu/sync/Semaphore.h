#pragma once

#include "gpu/Runtime.fwd.h"
#include <algorithm>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace gpu {

class Semaphore {
public:
  Semaphore(const Runtime &runtime, vk::SemaphoreCreateFlags flags = {});
  ~Semaphore() {
    if (m_semaphore) {
      m_device.destroySemaphore(m_semaphore);
    }
    m_device = VK_NULL_HANDLE;
    m_semaphore = VK_NULL_HANDLE;
  }
  Semaphore(const Semaphore &) = delete;
  Semaphore &operator=(const Semaphore &) = delete;
  Semaphore(Semaphore &&o) noexcept
      : m_device(std::move(o.m_device)), m_semaphore(std::move(o.m_semaphore)) {
    o.m_device = VK_NULL_HANDLE;
    o.m_semaphore = VK_NULL_HANDLE;
  }
  Semaphore &operator=(Semaphore &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~Semaphore();
    std::swap(m_device, o.m_device);
    std::swap(m_semaphore, o.m_semaphore);
    return *this;
  }

  inline vk::Semaphore handle() const {
    return m_semaphore;
  }

private:
  vk::Device m_device;
  vk::Semaphore m_semaphore;
};

} // namespace gpu
