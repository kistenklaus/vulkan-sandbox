#pragma once
#include "gpu/Runtime.h"
#include <ranges>
#include "gpu/descriptors/DescriptorSetLayout.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>

namespace gpu {

class PipelineLayout {
public:
  template <typename Allocator = std::allocator<vk::DescriptorSetLayout>>
  PipelineLayout(const Runtime& runtime, 
                 std::initializer_list<gpu::DescriptorSetLayout*> descriptorSetLayouts,
                 const Allocator alloc = {}) 
      : m_device(runtime.device) {
    vk::PipelineLayoutCreateInfo createInfo;
    std::vector<vk::DescriptorSetLayout> layouts(descriptorSetLayouts.size(), alloc);
    for (auto [i, setLayout] : descriptorSetLayouts | std::views::enumerate) {
      layouts[i] = setLayout->handle();
    }
    createInfo.setSetLayouts(layouts);
    vk::Result rCreatePipelineLayout =
        m_device.createPipelineLayout(&createInfo, nullptr, &m_layout);
    if (rCreatePipelineLayout != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create pipeline layout");
    }
  }
  ~PipelineLayout() {
    if (m_layout) {
      m_device.destroyPipelineLayout(m_layout);
      ;
    }
    m_device = VK_NULL_HANDLE;
    m_layout = VK_NULL_HANDLE;
  }
  PipelineLayout(const PipelineLayout &) = delete;
  PipelineLayout &operator=(const PipelineLayout &) = delete;
  PipelineLayout(PipelineLayout && o) noexcept : m_device(o.m_device), m_layout(o.m_layout){
    o.m_device = VK_NULL_HANDLE;
    o.m_layout = VK_NULL_HANDLE;
  }
  PipelineLayout &operator=(PipelineLayout && o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~PipelineLayout();
    std::swap(m_device, o.m_device);
    std::swap(m_layout, o.m_layout);
    return *this;
  }

  inline vk::PipelineLayout handle() const {
    return m_layout;
  }

  inline const vk::PipelineLayout* handlePtr() const {
    return &m_layout;
  }

private:
  vk::Device m_device;
  vk::PipelineLayout m_layout;
};

} // namespace gpu
