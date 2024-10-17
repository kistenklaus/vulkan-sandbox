#pragma once

#include "gpu/Runtime.h"
#include "gpu/cmd/CommandBuffer.h"
#include "gpu/pipeline/Pipeline.h"
#include "gpu/pipeline/PipelineLayout.h"
#include <io.h>
#include <memory_resource>
#include <spirv/compile.h>
#include <spirv/reflect.h>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace gpu {

class ComputePipeline : public Pipeline {
public:
  ComputePipeline() = delete;

  ComputePipeline(const Runtime &runtime,
                  const std::string &computeShaderFilePath,
                  const PipelineLayout *pipelineLayout)
      : Pipeline(), m_device(runtime.device) {

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

    vk::ComputePipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.stage.setModule(computeShaderModule);
    pipelineCreateInfo.stage.setStage(vk::ShaderStageFlagBits::eCompute);
    pipelineCreateInfo.stage.setPName("main");
    pipelineCreateInfo.setLayout(pipelineLayout->handle());
    vk::Result rCreateComputePipeline = m_device.createComputePipelines(
        VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
    if (rCreateComputePipeline != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create compute pipeline");
    }

    m_device.destroyShaderModule(computeShaderModule);
  }

  ~ComputePipeline() override {
    if (m_pipeline) {
      m_device.destroyPipeline(m_pipeline);
    }
  }
  ComputePipeline(const ComputePipeline &) = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;
  ComputePipeline(ComputePipeline &&o) noexcept
      : Pipeline(o.m_pipeline), m_device(std::move(o.m_device)) {
    o.m_device = VK_NULL_HANDLE;
    o.m_pipeline = VK_NULL_HANDLE;
  }
  ComputePipeline &operator=(ComputePipeline &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~ComputePipeline();
    std::swap(m_device, o.m_device);
    std::swap(m_pipeline, o.m_pipeline);
    return *this;
  }

private:
  vk::Device m_device;
};

} // namespace gpu
