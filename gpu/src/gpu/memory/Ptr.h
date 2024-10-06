#pragma once

#include <vulkan/vulkan.hpp>

namespace gpu::memory {

struct Ptr {
  vk::DeviceMemory deviceMemory;
  vk::DeviceSize offset;

  operator bool() const {
    return static_cast<bool>(deviceMemory);
  }
};

};
