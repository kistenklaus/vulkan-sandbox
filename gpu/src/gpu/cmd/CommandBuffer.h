#pragma once

#include "gpu/cmd/CommandPool.fwd.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace gpu {

class CommandBuffer {
public:
  CommandBuffer() = delete;
  CommandBuffer(vk::CommandBuffer cmd, cmd::CommandPoolHandles *poolHandles);
  ~CommandBuffer();
  CommandBuffer(const CommandBuffer &) = delete;
  CommandBuffer &operator=(const CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) noexcept;
  CommandBuffer &operator=(CommandBuffer &&) noexcept;

  void begin();
  void end();

  vk::CommandBuffer cmd;

private:
  cmd::CommandPoolHandles *m_weakHandles;
};

} // namespace gpu::cmd
