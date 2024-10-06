#include "./CommandBuffer.h"
#include "gpu/cmd/CommandPool.h"
#include <vulkan/vulkan_core.h>

using namespace gpu;

CommandBuffer::CommandBuffer(vk::CommandBuffer cmd,
                             cmd::CommandPoolHandles *poolHandles)
    : cmd(cmd), m_weakHandles(poolHandles) {}
CommandBuffer::~CommandBuffer() {
  if (m_weakHandles) {
    m_weakHandles->device.freeCommandBuffers(m_weakHandles->pool, cmd);
  }
}
CommandBuffer::CommandBuffer(CommandBuffer &&o) noexcept
    : cmd(std::move(o.cmd)), m_weakHandles(std::move(o.m_weakHandles)) {
  o.cmd = VK_NULL_HANDLE;
  o.m_weakHandles = VK_NULL_HANDLE;
}
CommandBuffer &CommandBuffer::operator=(CommandBuffer &&o) noexcept {
  if (this == &o) {
    return *this;
  }
  this->~CommandBuffer();
  std::swap(cmd, o.cmd);
  std::swap(m_weakHandles, o.m_weakHandles);
  return *this;
}

void CommandBuffer::begin() {
  vk::CommandBufferBeginInfo beginInfo;
  cmd.begin(beginInfo);
}

void CommandBuffer::end() { cmd.end(); }
