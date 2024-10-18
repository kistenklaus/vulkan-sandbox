#pragma once
#include "gpu/gpu.h"

class DecoupledLookBackPrefixSum {
public:
  DecoupledLookBackPrefixSum(bool validationLayer = false);
  ~DecoupledLookBackPrefixSum();
  std::vector<unsigned int> update();

  inline gpu::time::Clock::duration duration() const { return m_duration; }

  static constexpr size_t WEIGHT_COUNT = 64e6;
private:
  gpu::Runtime m_runtime;
  gpu::time::Clock::duration m_duration;
  gpu::pmr::CommandPool m_commandPool;
  gpu::memory::MonotonicResource m_deviceLocalMemory;
  gpu::memory::MonotonicResource m_hostVisibleMemory;
  std::byte m_heap[4096];
  gpu::DescriptorSetLayout m_set0Layout;
  gpu::PipelineLayout m_pipelineLayout;
  gpu::ComputePipeline m_computePipeline;
  gpu::DescriptorPool m_descriptorPool;
  gpu::DescriptorSet m_set0;
  gpu::Timer m_timer;
  gpu::Fence m_fence;
  gpu::Fence m_transferFence;
};

