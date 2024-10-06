#pragma once

#include "gpu/Runtime.h"
#include "gpu/cmd/CommandBuffer.h"
#include "gpu/descriptors/DescriptorPool.h"
#include <cassert>
#include <io.h>
#include <memory>
#include <memory_resource>
#include <ranges>
#include <spirv/compile.h>
#include <spirv/reflect.h>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace gpu {

template <typename DescriptorSetLayoutAllocator =
              std::allocator<vk::DescriptorSetLayout>,
          typename DescriptorPoolSizeAllocator =
              std::allocator<vk::DescriptorPoolSize>>
class ComputePipeline {
public:
  ComputePipeline() = delete;

  ComputePipeline(
      const Runtime &runtime, const std::string &computeShaderFilePath,
      uint32_t maxPipelineDescriptorCount = 1,
      const DescriptorSetLayoutAllocator &descriptorSetLayoutAlloc = {},
      const DescriptorPoolSizeAllocator &descriptorPoolSizeAlloc = {})
      : m_device(runtime.device),
        m_descriptorSetLayouts(descriptorSetLayoutAlloc),
        m_descriptorPoolSizes(descriptorPoolSizeAlloc) {

    std::byte buffer[1024];
    std::pmr::monotonic_buffer_resource bufferResource{buffer, sizeof(buffer)};

    const std::vector<std::byte> glslSource =
        io::readFileSync(computeShaderFilePath);
    const std::vector<uint32_t> spirvSource =
        spirv::compileShader(glslSource, computeShaderFilePath);

    vk::ShaderModuleCreateInfo computeShaderCreateInfo;
    computeShaderCreateInfo.setCode(spirvSource);
    vk::ShaderModule computeShaderModule;
    vk::Result rCreateComputeShaderModule = m_device.createShaderModule(
        &computeShaderCreateInfo, nullptr, &computeShaderModule);
    if (rCreateComputeShaderModule != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create compute shader module");
    }

    SpvReflectShaderModule reflectComputeShader;
    SpvReflectResult rReflect = spvReflectCreateShaderModule(
        spirvSource.size() * 4, spirvSource.data(), &reflectComputeShader);

    if (rReflect != SPV_REFLECT_RESULT_SUCCESS) {
      throw std::runtime_error("Failed to reflect on shader " +
                               computeShaderFilePath);
    }

    assert((reflectComputeShader.shader_stage &
            SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT) != 0);

    std::span<SpvReflectDescriptorSet> reflectDescriptorSets;
    {
      uint32_t count;
      spvReflectEnumerateDescriptorSets(&reflectComputeShader, &count, nullptr);
      SpvReflectDescriptorSet *buffer;
      spvReflectEnumerateDescriptorSets(&reflectComputeShader, &count, &buffer);
      reflectDescriptorSets = {buffer, count};
    }

    std::pmr::vector<vk::DescriptorPoolSize> descriptorSetPoolSizes;
    m_descriptorSetLayouts.resize(reflectDescriptorSets.size());
    uint32_t totalDescriptor = 0;
    for (auto [i, set] : reflectDescriptorSets | std::views::enumerate) {
      assert(i == set.set);
      totalDescriptor += set.binding_count;
      std::pmr::vector<vk::DescriptorSetLayoutBinding> layoutBindings(
          set.binding_count);
      std::span reflectBindings(set.bindings, set.binding_count);

      for (auto [i, reflect] : reflectBindings | std::views::enumerate) {
        vk::DescriptorType type = vk::DescriptorType(reflect->resource_type);
        bool inc = false;
        for (size_t i = 0; i < descriptorSetPoolSizes.size(); ++i) {
          if (descriptorSetPoolSizes[i].type == type) {
            descriptorSetPoolSizes[i].descriptorCount += 1;
            inc = true;
            break;
          }
        }
        if (!inc) {
          std::tuple<vk::DescriptorType, uint32_t> pair{type, 1};
          descriptorSetPoolSizes.emplace_back(type, 1);
        }
        layoutBindings[i].setBinding(reflect->binding);
        layoutBindings[i].setStageFlags(vk::ShaderStageFlagBits::eCompute);
        layoutBindings[i].setDescriptorType(type);
        assert(reflect->count == 1);
        layoutBindings[i].setDescriptorCount(reflect->count);
      }
      vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
      setLayoutCreateInfo.setBindings(layoutBindings);

      vk::Result rCreateDescriptorSetLayout =
          m_device.createDescriptorSetLayout(&setLayoutCreateInfo, nullptr,
                                             m_descriptorSetLayouts.data() + i);
      if (rCreateDescriptorSetLayout != vk::Result::eSuccess) {
        throw std::runtime_error("Faild to create descriptor set layout");
      }
    }
    m_descriptorPoolSizes.assign(descriptorSetPoolSizes);

    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setSetLayouts(m_descriptorSetLayouts);
    vk::Result rCreateLayout = m_device.createPipelineLayout(
        &layoutCreateInfo, nullptr, &m_pipelineLayout);
    if (rCreateLayout != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create compute pipeline layout");
    }

    spvReflectDestroyShaderModule(&reflectComputeShader);

    vk::ComputePipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.stage.setModule(computeShaderModule);
    pipelineCreateInfo.stage.setStage(vk::ShaderStageFlagBits::eCompute);
    pipelineCreateInfo.stage.setPName("main");
    pipelineCreateInfo.setLayout(m_pipelineLayout);
    vk::Result rCreateComputePipeline = m_device.createComputePipelines(
        VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    if (rCreateComputePipeline != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create compute pipeline");
    }

    m_device.destroyShaderModule(computeShaderModule);
  }

  void createDescriptorPool() {}

  ~ComputePipeline() {
    m_device.destroyPipeline(m_pipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);
    for (auto descriptorSetLayout : m_descriptorSetLayouts) {
      m_device.destroyDescriptorSetLayout(descriptorSetLayout);
    }
  }
  ComputePipeline(const ComputePipeline &) = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;
  ComputePipeline(ComputePipeline &&o) noexcept
      : m_device(std::move(o.m_device)), m_pipeline(std::move(o.m_pipeline)),
        m_pipelineLayout(std::move(o.m_pipelineLayout)),
        m_descriptorSetLayouts(std::move(o.m_descriptorSetLayouts)) {
    o.m_device = VK_NULL_HANDLE;
    o.m_pipeline = VK_NULL_HANDLE;
    o.m_pipelineLayout = VK_NULL_HANDLE;
    o.m_descriptorSetLayouts = VK_NULL_HANDLE;
    o.m_descriptorPool = VK_NULL_HANDLE;
  }
  ComputePipeline &operator=(ComputePipeline &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~ComputePipeline();
    std::swap(m_device, o.m_device);
    std::swap(m_pipeline, o.m_pipeline);
    std::swap(m_pipelineLayout, o.m_pipelineLayout);
    std::swap(m_descriptorSetLayouts, o.m_descriptorSetLayouts);
    return *this;
  }

  void dispatch(const CommandBuffer &commandBuffer, uint32_t groupCountX,
                uint32_t groupCountY, uint32_t groupCountZ) {
    commandBuffer.cmd.dispatch(groupCountX, groupCountY, groupCountZ);
  }

  /**
   * count is NOT the number of descriptors that can be allocated from the pool
   * it is the amount of times complete descriptor sets for this pipeline can be
   * allocated.
   */
  DescriptorPool createDescriptorPool(size_t count) {
    vk::DescriptorPoolSize buffer[5];
    std::pmr::monotonic_buffer_resource bufferResource{buffer, sizeof(buffer)};
    std::pmr::vector<vk::DescriptorPoolSize> poolSizes(m_descriptorPoolSizes,
                                                       &bufferResource);
    uint32_t totalCount = 0;
    for (vk::DescriptorPoolSize &size : poolSizes) {
      size.descriptorCount *= count;
      totalCount += size.descriptorCount;
    }

    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.setPoolSizes(poolSizes);
    poolCreateInfo.setMaxSets(totalCount);
    vk::DescriptorPool pool;
    vk::Result rCreateDescriptorPool =
        m_device.createDescriptorPool(&poolCreateInfo, nullptr, &pool);
    if (rCreateDescriptorPool != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create descriptor pool");
    }
    return DescriptorPool(m_device, pool);
  }

private:
  vk::Device m_device;
  vk::Pipeline m_pipeline;
  vk::PipelineLayout m_pipelineLayout;
  std::vector<vk::DescriptorSetLayout, DescriptorSetLayoutAllocator>
      m_descriptorSetLayouts;
  std::vector<vk::DescriptorPoolSize, DescriptorPoolSizeAllocator>
      m_descriptorPoolSizes;
};

namespace pmr {
using ComputePipeline =
    ComputePipeline<std::pmr::polymorphic_allocator<vk::DescriptorSetLayout>,
                    std::pmr::polymorphic_allocator<vk::DescriptorPoolSize>>;
}

} // namespace gpu
