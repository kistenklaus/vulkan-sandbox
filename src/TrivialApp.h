#pragma once
#include "gpu/gpu.h"

class TrivialApp {
public:
  TrivialApp(bool validationLayer = false);
  ~TrivialApp();
  void update();

  inline gpu::time::Clock::duration duration() const { return m_duration; }

private:
  static constexpr size_t WEIGHT_COUNT = 1e6;
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

