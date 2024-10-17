#include "./CommandBuffer.h"
#include "gpu/cmd/CommandPool.h"
#include <vulkan/vulkan_core.h>
#include "gpu/pipeline/PipelineLayout.h"
#include "gpu/descriptors/DescriptorSet.h"

using namespace gpu;

CommandBuffer::CommandBuffer(vk::CommandBuffer cmd,
                             cmd::CommandPoolHandles *poolHandles)
    : m_commandBuffer(cmd), m_weakHandles(poolHandles) {}
CommandBuffer::~CommandBuffer() {
  if (m_weakHandles) {
    m_weakHandles->device.freeCommandBuffers(m_weakHandles->pool, m_commandBuffer);
  }
}
CommandBuffer::CommandBuffer(CommandBuffer &&o) noexcept
    : m_commandBuffer(std::move(o.m_commandBuffer)), m_weakHandles(std::move(o.m_weakHandles)) {
  o.m_commandBuffer = VK_NULL_HANDLE;
  o.m_weakHandles = VK_NULL_HANDLE;
}
CommandBuffer &CommandBuffer::operator=(CommandBuffer &&o) noexcept {
  if (this == &o) {
    return *this;
  }
  this->~CommandBuffer();
  std::swap(m_commandBuffer, o.m_commandBuffer);
  std::swap(m_weakHandles, o.m_weakHandles);
  return *this;
}

void CommandBuffer::begin() {
  vk::CommandBufferBeginInfo beginInfo;
  m_commandBuffer.begin(beginInfo);
}

void CommandBuffer::end() { m_commandBuffer.end(); }
void gpu::CommandBuffer::bindDescriptorSet(
    const DescriptorSet *set, vk::PipelineBindPoint bindpoint,
    const PipelineLayout *pipelineLayout) {
  m_commandBuffer.bindDescriptorSets(bindpoint, pipelineLayout->handle(), 0, 1,
                         set->handlePtr(), 0, nullptr);
}

