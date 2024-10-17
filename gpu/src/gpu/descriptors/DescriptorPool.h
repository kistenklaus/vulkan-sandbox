#pragma once
#include <iostream>
#include "gpu/Runtime.h"
#include "gpu/descriptors/DescriptorSet.h"
#include "gpu/descriptors/DescriptorSetLayout.h"
#include <algorithm>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>

namespace gpu {

template <typename Allocator> class DescriptorPoolBuilder;

class DescriptorPool {
public:
  DescriptorPool(vk::Device device, vk::DescriptorPool pool,
                 vk::DescriptorPoolCreateFlags flags)
      : m_device(device), m_pool(pool),
        m_free(flags & vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet) {}
  ~DescriptorPool() {
    if (m_pool) {
      m_device.destroyDescriptorPool(m_pool);
    }
  }
  DescriptorPool(const DescriptorPool &) = delete;
  DescriptorPool &operator=(const DescriptorPool &) = delete;
  DescriptorPool(DescriptorPool &&o) noexcept
      : m_device(std::move(o.m_device)), m_pool(std::move(o.m_pool)) {
    o.m_pool = VK_NULL_HANDLE;
    o.m_device = VK_NULL_HANDLE;
  }
  DescriptorPool &operator=(DescriptorPool &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~DescriptorPool();
    std::swap(m_device, o.m_device);
    std::swap(m_pool, o.m_pool);
    return *this;
  }

  DescriptorSet alloc(const DescriptorSetLayout *layout) {
    vk::DescriptorSetAllocateInfo allocInfo;
    vk::DescriptorSetLayout layoutHandle = layout->handle();
    allocInfo.setSetLayouts(layoutHandle);
    allocInfo.setDescriptorPool(m_pool);
    vk::DescriptorSet set;
    vk::Result rAllocDescriptorSet =
        m_device.allocateDescriptorSets(&allocInfo, &set);
    if (rAllocDescriptorSet != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to allocate descriptor set");
    }
    return DescriptorSet(m_device, m_free ? m_pool : VK_NULL_HANDLE, set);
  }

  template <typename Allocator = std::allocator<vk::DescriptorPoolSize>>
  static DescriptorPoolBuilder<Allocator> builder(size_t poolSizesHint = 3,
                                                  const Allocator &alloc = {});

  inline vk::DescriptorPool handle() const { return m_pool; }

private:
  vk::Device m_device;
  vk::DescriptorPool m_pool;
  bool m_free;
};

template <typename Allocator = std::allocator<vk::DescriptorPoolSize>>
class DescriptorPoolBuilder {
public:
  DescriptorPoolBuilder(size_t poolSizesHint, const Allocator &alloc = {})
      : m_poolSizes(alloc), m_totalSets(0) {
    m_poolSizes.reserve(poolSizesHint);
  }

  DescriptorPoolBuilder &require(size_t count, vk::DescriptorType type) {
    auto it = std::ranges::find_if(m_poolSizes,
                                   [type](vk::DescriptorPoolSize poolSize) {
                                     return type == poolSize.type;
                                   });
    if (it == m_poolSizes.end()) {
      vk::DescriptorPoolSize poolSize;
      poolSize.setType(type);
      poolSize.setDescriptorCount(count);
      m_poolSizes.emplace_back(std::move(poolSize));
    } else {
      it->descriptorCount += count;
    }
    m_totalSets += count;
    return *this;
  }

  DescriptorPoolBuilder &require(size_t count,
                                 const DescriptorSetLayout *layout) {
    uint32_t maxBinding = layout->maxBinding();
    for (size_t binding = 0; binding <= maxBinding; ++binding) {
      auto descriptorTypeOpt = layout->typeOf(binding);
      if (!descriptorTypeOpt) {
        continue;
      }
      require(count, *descriptorTypeOpt);
    }
    return *this;
  }

  DescriptorPool complete(const Runtime &runtime) {
    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(m_poolSizes);
    createInfo.setMaxSets(m_totalSets);
    vk::DescriptorPool pool;
    vk::Result rCreateDescriptorPool =
        runtime.device.createDescriptorPool(&createInfo, nullptr, &pool);
    if (rCreateDescriptorPool != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create descriptor pool");
    }
    return DescriptorPool(runtime.device, pool, createInfo.flags);
  }

private:
  std::vector<vk::DescriptorPoolSize, Allocator> m_poolSizes;
  uint32_t m_totalSets;
};

template <typename Allocator>
DescriptorPoolBuilder<Allocator>
DescriptorPool::builder(size_t poolSizesHint, const Allocator &alloc) {
  return {poolSizesHint, alloc};
}

} // namespace gpu
