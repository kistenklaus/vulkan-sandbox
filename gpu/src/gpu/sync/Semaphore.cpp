#include "./Semaphore.h"
#include "gpu/Runtime.h"
#include <stdexcept>

gpu::Semaphore::Semaphore(const Runtime &runtime,
                          vk::SemaphoreCreateFlags flags)
    : m_device(runtime.device) {
  vk::SemaphoreCreateInfo createInfo{flags};
  vk::Result rCreateSemaphore =
      m_device.createSemaphore(&createInfo, nullptr, &m_semaphore);
  if (rCreateSemaphore != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create semaphore");
  }
}

