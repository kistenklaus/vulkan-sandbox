#pragma once

#include <cassert>
#include <vulkan/vulkan.hpp>
#include "gpu/Queue.h"
#include "gpu/memory/DeviceMemoryResource.h"

namespace gpu {

struct RuntimeCreateInfo {
  std::optional<std::string> applicationName;
  std::optional<std::tuple<uint32_t, uint32_t, uint32_t>> applicationVersion;
  bool validationLayer = true;
};

class Runtime {
public:
  Runtime(const RuntimeCreateInfo &createInfo = {});
  ~Runtime();
  Runtime(const Runtime &) = delete;
  Runtime &operator=(const Runtime &) = delete;
  Runtime(Runtime &&) noexcept;
  Runtime &operator=(Runtime &&) noexcept;

public:
  vk::Instance instance;
  vk::PhysicalDevice physicalDevice;
  vk::Device device;

  Queue graphicsQueue;
  Queue computeQueue;

  inline gpu::memory::DeviceMemoryResource* deviceLocalMemory() {
    assert(m_deviceLocalMemory);
    return const_cast<gpu::memory::DeviceMemoryResource*>(&*m_deviceLocalMemory);
  }
  
  inline gpu::memory::DeviceMemoryResource* hostVisibleMemory() {
    assert(m_deviceLocalMemory);
    return &*m_hostVisibleMemory;
  }

private:
  std::optional<gpu::memory::DeviceMemoryResource> m_deviceLocalMemory;
  std::optional<gpu::memory::DeviceMemoryResource> m_hostVisibleMemory;
  vk::DebugUtilsMessengerEXT debugUtilsMessenger;
};

}
