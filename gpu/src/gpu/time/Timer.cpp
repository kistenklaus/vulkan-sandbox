#include "./Timer.h"
#include "gpu/Runtime.h"
#include <vulkan/vulkan_core.h>

gpu::Timer::Timer(const Runtime &runtime, size_t samples)
    : m_device(runtime.device), m_queryCount(samples) {
  vk::PhysicalDeviceProperties props = runtime.physicalDevice.getProperties();
  assert(props.limits.timestampComputeAndGraphics == VK_TRUE);
  m_timestampPeriod = props.limits.timestampPeriod;
  vk::QueryPoolCreateInfo queryPoolCreateInfo;
  queryPoolCreateInfo.setQueryType(vk::QueryType::eTimestamp);
  queryPoolCreateInfo.setQueryCount(samples);

  vk::Result rCreateQueryPool =
      m_device.createQueryPool(&queryPoolCreateInfo, nullptr, &m_queryPool);
  if (rCreateQueryPool != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create query pool");
  }
}
