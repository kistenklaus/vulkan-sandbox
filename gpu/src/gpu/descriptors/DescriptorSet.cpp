#include "./DescriptorSet.h"
#include "gpu/compute/ComputePipeline.h"
#include <vulkan/vulkan_core.h>

using namespace gpu;

DescriptorSet::~DescriptorSet() {
  if (static_cast<bool>(m_weakHandles->pool) &&
      m_descriptorSet != VK_NULL_HANDLE) {
    m_weakHandles->device.freeDescriptorSets(m_weakHandles->pool, 1,
                                             &m_descriptorSet);
  }
}

DescriptorSet::DescriptorSet(vk::DescriptorSet descriptorSet,
                             ComputePipelineHandles *weakHandles)
    : m_descriptorSet(descriptorSet), m_weakHandles(weakHandles) {}

DescriptorSet::DescriptorSet(DescriptorSet &&o) noexcept
    : m_descriptorSet(std::move(o.m_descriptorSet)),
      m_weakHandles(std::move(o.m_weakHandles)) {
  o.m_weakHandles = nullptr;
  o.m_descriptorSet = VK_NULL_HANDLE;
}

DescriptorSet &DescriptorSet::operator=(DescriptorSet &&o) noexcept {
  if (this == &o) {
    return *this;
  }
  this->~DescriptorSet();
  std::swap(m_descriptorSet, o.m_descriptorSet);
  std::swap(m_weakHandles, o.m_weakHandles);
  return *this;
}

void DescriptorSet::write(uint32_t binding, const Buffer& buffer) {
  vk::WriteDescriptorSet writeInfo;
  writeInfo.setDstBinding(binding);
  writeInfo.setDstSet(m_descriptorSet);
  writeInfo.setDstArrayElement(0);
  vk::DescriptorBufferInfo bufferInfo;
  bufferInfo.setBuffer(buffer.m_buffer);
  bufferInfo.setRange(VK_WHOLE_SIZE);
  bufferInfo.setOffset(0);
  writeInfo.setBufferInfo(bufferInfo);
  m_weakHandles->device.updateDescriptorSets(1, &writeInfo, 0, nullptr);
}
