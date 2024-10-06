#pragma once

#include "gpu/buffer/Buffer.h"
#include <vulkan/vulkan.hpp>

namespace gpu {

class DescriptorSet {
public:
  DescriptorSet(vk::DescriptorSet descriptorSet);
  ~DescriptorSet();
  DescriptorSet(const DescriptorSet &) = delete;
  DescriptorSet &operator=(const DescriptorSet &) = delete;
  DescriptorSet(DescriptorSet &&) noexcept;
  DescriptorSet &operator=(DescriptorSet &&) noexcept;

  template <typename GpuAllocator = memory::PolymorphicAllocator>
  void write(uint32_t binding, const Buffer<GpuAllocator> &buffer) {
    vk::WriteDescriptorSet writeInfo;
    writeInfo.setDstBinding(binding);
    writeInfo.setDstSet(m_descriptorSet);
    writeInfo.setDstArrayElement(0);
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.setBuffer(buffer.m_buffer);
    bufferInfo.setRange(VK_WHOLE_SIZE);
    bufferInfo.setOffset(0);
    writeInfo.setBufferInfo(bufferInfo);
    m_device.updateDescriptorSets(1, &writeInfo, 0, nullptr);
  }

private:
  vk::DescriptorPool m_pool;
  vk::Device m_device;
  vk::DescriptorSet m_descriptorSet;
};

} // namespace gpu
