#pragma once

#include "gpu/time/Timer.h"
#include "gpu/buffer/Buffer.h"
#include "gpu/cmd/CommandPool.fwd.h"
#include "gpu/descriptors/DescriptorSet.fwd.h"
#include "gpu/memory/PolymorphicAllocator.h"
#include "gpu/pipeline/Pipeline.h"
#include "gpu/pipeline/PipelineLayout.fwd.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>

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

  void bindPipeline(const Pipeline *pipeline, vk::PipelineBindPoint bindpoint) {
    m_commandBuffer.bindPipeline(bindpoint, pipeline->handle());
  }

  void bindDescriptorSet(const DescriptorSet *set,
                         vk::PipelineBindPoint bindpoint,
                         const PipelineLayout *pipelineLayout);

  void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) {
    m_commandBuffer.dispatch(x, y, z);
  }

  void copyBuffer(const Buffer *dst, const Buffer *src,
                  std::optional<size_t> size = std::nullopt) {
    vk::DeviceSize copySize =
        size.has_value() ? static_cast<vk::DeviceSize>(*size) : src->size();
    vk::BufferCopy copyInfo;
    copyInfo.setSize(copySize);
    m_commandBuffer.copyBuffer(src->handle(), dst->handle(), 1, &copyInfo);
  }

  inline vk::CommandBuffer handle() const { return m_commandBuffer; }

  void resetTimer(Timer* timer) {
    timer->reset();
    m_commandBuffer.resetQueryPool(timer->handle(), 0, timer->queryCount());
  }

  void sampleTimestamp(vk::PipelineStageFlagBits pipelineStage, Timer* timer) {
    m_commandBuffer.writeTimestamp(pipelineStage, timer->handle(), timer->nextQuery());
  }

private:
  vk::CommandBuffer m_commandBuffer;
  cmd::CommandPoolHandles *m_weakHandles;
};

} // namespace gpu
