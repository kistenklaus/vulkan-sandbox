#pragma once
#include "gpu/Runtime.h"
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gpu {

template <typename Allocator> class DescriptorSetLayoutBuilder;

class DescriptorSetLayout {
public:
  DescriptorSetLayout(vk::Device device, vk::DescriptorSetLayout layout,
                      std::vector<std::optional<vk::DescriptorType>> &&types)
      : m_device(device), m_layout(layout), m_types(std::move(types)) {
  }
  ~DescriptorSetLayout() {
    if (m_layout) {
      m_device.destroyDescriptorSetLayout(m_layout);
    }
    m_device = VK_NULL_HANDLE;
    m_layout = VK_NULL_HANDLE;
    m_types.clear();
  }
  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

  DescriptorSetLayout(DescriptorSetLayout &&o) noexcept
      : m_device(std::move(o.m_device)), m_layout(std::move(o.m_layout)), m_types(std::move(o.m_types)) {
    o.m_device = VK_NULL_HANDLE;
    o.m_layout = VK_NULL_HANDLE;
  }
  DescriptorSetLayout &operator=(DescriptorSetLayout &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~DescriptorSetLayout();
    std::swap(m_device, o.m_device);
    std::swap(m_layout, o.m_layout);
    std::swap(m_types, o.m_types);
    return *this;
  }

  inline vk::DescriptorSetLayout handle() const { return m_layout; }

  template <typename Allocator = std::allocator<vk::DescriptorSetLayoutBinding>>
  static DescriptorSetLayoutBuilder<Allocator>
  builder(size_t bindingCountHint = 3, const Allocator &alloc = {});

  inline std::optional<vk::DescriptorType> typeOf(uint32_t binding) const {
    return m_types[binding];
  }

  inline uint32_t maxBinding() const { return m_types.size() - 1; }

private:
  vk::Device m_device;
  vk::DescriptorSetLayout m_layout;
  std::vector<std::optional<vk::DescriptorType>> m_types;
};

template <typename Allocator = std::allocator<vk::DescriptorSetLayoutBinding>>
class DescriptorSetLayoutBuilder {
public:
  DescriptorSetLayoutBuilder(size_t bindingCountHint,
                             const Allocator &alloc = {})
      : m_bindings(alloc) {
    m_bindings.reserve(bindingCountHint);
  }
  DescriptorSetLayoutBuilder &addBinding(size_t binding,
                                         vk::DescriptorType type,
                                         vk::ShaderStageFlags stages) {
    // ensure that bindings doesn't contian duplicated bindings.
    assert([&]() {
      return std::ranges::none_of(m_bindings,
                                  [&](auto b) { return b.binding == binding; });
    }());
    vk::DescriptorSetLayoutBinding layoutBinding;
    layoutBinding.setBinding(binding);
    layoutBinding.setDescriptorType(type);
    layoutBinding.setDescriptorCount(1);
    layoutBinding.setStageFlags(stages);
    m_bindings.push_back(layoutBinding);
    return *this;
  }

  DescriptorSetLayout complete(const Runtime &runtime) {
    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.setBindings(m_bindings);
    vk::DescriptorSetLayout layout;
    vk::Result rCreateDescriptorLayout =
        runtime.device.createDescriptorSetLayout(&createInfo, nullptr, &layout);
    if (rCreateDescriptorLayout != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create descriptor set layout");
    }
    int32_t maxBinding = -1;
    for (const vk::DescriptorSetLayoutBinding &binding : m_bindings) {
      maxBinding = std::max(static_cast<int32_t>(binding.binding), maxBinding);
    }
    std::vector<std::optional<vk::DescriptorType>> types(maxBinding + 1);
    for (const vk::DescriptorSetLayoutBinding &binding : m_bindings) {
      types[binding.binding] = binding.descriptorType;
    }
    return DescriptorSetLayout(runtime.device, layout, std::move(types));
  }

private:
  std::vector<vk::DescriptorSetLayoutBinding, Allocator> m_bindings;
};

template <typename Allocator>
DescriptorSetLayoutBuilder<Allocator>
DescriptorSetLayout::builder(size_t sizeHint, const Allocator &alloc) {
  return {sizeHint, alloc};
}

} // namespace gpu
  //
  //
