#pragma once

#include "gpu/buffer/Buffer.h"
#include "gpu/descriptors/DescriptorSetLayout.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace gpu {

class DescriptorSet {
public:
  DescriptorSet(vk::Device device, vk::DescriptorPool pool,
                vk::DescriptorSet descriptorSet)
      : m_device(device), m_pool(pool), m_descriptorSet(descriptorSet) {}
  ~DescriptorSet() {
    if (m_pool != VK_NULL_HANDLE && m_descriptorSet != VK_NULL_HANDLE) {
      m_device.freeDescriptorSets(m_pool, m_descriptorSet);
    }
    m_device = VK_NULL_HANDLE;
    m_pool = VK_NULL_HANDLE;
    m_descriptorSet = VK_NULL_HANDLE;
  }
  DescriptorSet(const DescriptorSet &) = delete;
  DescriptorSet &operator=(const DescriptorSet &) = delete;
  DescriptorSet(DescriptorSet &&o) noexcept
      : m_device(std::move(o.m_device)), m_pool(std::move(o.m_pool)),
        m_descriptorSet(std::move(o.m_descriptorSet)) {
    o.m_device = VK_NULL_HANDLE;
    o.m_pool = VK_NULL_HANDLE;
    o.m_descriptorSet = VK_NULL_HANDLE;
  }
  DescriptorSet &operator=(DescriptorSet &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~DescriptorSet();
    std::swap(m_device, o.m_device);
    std::swap(m_pool, o.m_pool);
    std::swap(m_descriptorSet, o.m_descriptorSet);
    return *this;
  }

  void write(uint32_t binding, const Buffer &buffer,
             vk::DescriptorType type) {
    vk::WriteDescriptorSet writeInfo;
    writeInfo.setDstBinding(binding);
    writeInfo.setDstSet(m_descriptorSet);
    writeInfo.setDstArrayElement(0);
    writeInfo.setDescriptorType(type);
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.setBuffer(buffer.m_buffer);
    bufferInfo.setRange(VK_WHOLE_SIZE);
    bufferInfo.setOffset(0);
    writeInfo.setBufferInfo(bufferInfo);
    m_device.updateDescriptorSets(1, &writeInfo, 0, nullptr);
  }

  void write(uint32_t binding, const Buffer &buffer,
             const DescriptorSetLayout *layout) {
    std::optional<vk::DescriptorType> type = layout->typeOf(binding);
    assert(type);
    write(binding, buffer, *type);
  }

  inline vk::DescriptorSet handle() const { return m_descriptorSet; }
  inline const vk::DescriptorSet* handlePtr() const { return &m_descriptorSet; }

private:
  vk::DescriptorPool m_pool;
  vk::Device m_device;
  vk::DescriptorSet m_descriptorSet;
};

} // namespace gpu
