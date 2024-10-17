#include "./Fence.h"
#include "gpu/Runtime.h"

gpu::Fence::Fence(const Runtime &runtime, vk::FenceCreateFlags flags)
    : m_device(runtime.device) {
  vk::FenceCreateInfo createInfo{flags};
  vk::Result rCreateFence =
      m_device.createFence(&createInfo, nullptr, &m_fence);
  if (rCreateFence != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create fence");
  }
}

