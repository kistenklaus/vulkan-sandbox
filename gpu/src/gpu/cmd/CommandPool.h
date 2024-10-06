#pragma once

#include "gpu/Runtime.h"
#include "gpu/cmd/CommandBuffer.h"
#include <memory_resource>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace gpu {

namespace cmd {

struct CommandPoolHandles {
  vk::Device device;
  vk::CommandPool pool;
};
} // namespace cmd

template <typename Allocator = std::allocator<cmd::CommandPoolHandles>>
class CommandPool {
private:
  using allocator_traits = std::allocator_traits<Allocator>;

public:
  CommandPool(const gpu::Runtime &runtime,
              vk::CommandPoolCreateFlags flags = {},
              const Allocator &allocator = {})
      : m_allocator(allocator) {
    m_ownedHandles = allocator_traits::allocate(m_allocator, 1);
    m_ownedHandles->device = runtime.device;
    vk::Device device = runtime.device;

    vk::CommandPoolCreateInfo createInfo;
    createInfo.setQueueFamilyIndex(runtime.graphicsQueueFamilyIndex);
    createInfo.setFlags(flags);
    vk::Result rCreateCommandPool =
        device.createCommandPool(&createInfo, nullptr, &m_ownedHandles->pool);
    if (rCreateCommandPool != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create command pool");
    }
  }
  ~CommandPool() {
    if (m_ownedHandles) {
      m_ownedHandles->device.destroyCommandPool(m_ownedHandles->pool);
      m_ownedHandles->device = VK_NULL_HANDLE;
      m_ownedHandles->pool = VK_NULL_HANDLE;
      allocator_traits::deallocate(m_allocator, m_ownedHandles, 1);
    }
  }

  CommandPool(const CommandPool &) = delete;
  CommandPool &operator=(const CommandPool &) = delete;

  CommandPool(CommandPool &&o) noexcept
      : m_ownedHandles(std::move(o.m_ownedHandles)),
        m_allocator(std::move(o.m_allocator)) {
    o.m_ownedHandles = nullptr;
  }

  CommandPool &operator=(CommandPool &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~CommandPool();
    std::swap(m_ownedHandles, o.m_ownedHandles);
    if constexpr (allocator_traits::propagate_on_container_move_assignment) {
      m_allocator = o.m_allocator;
    } else {
      m_allocator = {};
    }
    return *this;
  }

  CommandBuffer
  allocate(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandBufferCount(1);
    allocInfo.setCommandPool(m_ownedHandles->pool);
    allocInfo.setLevel(level);
    vk::CommandBuffer cmd;
    vk::Result rAllocCmd =
        m_ownedHandles->device.allocateCommandBuffers(&allocInfo, &cmd);
    if (rAllocCmd != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to allocate command buffer");
    }
    return CommandBuffer(cmd, m_ownedHandles);
  }

private:
  cmd::CommandPoolHandles *m_ownedHandles;
  [[no_unique_address]] Allocator m_allocator;
};

namespace pmr {
using CommandPool = CommandPool<std::pmr::polymorphic_allocator<cmd::CommandPoolHandles>>;
}

} // namespace gpu
