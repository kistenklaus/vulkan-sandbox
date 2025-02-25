#include "./DecoupledLookBackPrefixSum.h"
#include "gpu/Runtime.h"
#include "gpu/descriptors/DescriptorSetLayout.h"
#include <cmath>
#include <gpu/gpu.h>
#include <memory_resource>
#include <vulkan/vulkan_enums.hpp>

constexpr size_t WORK_GROUP_SIZE = 512;
constexpr size_t ROWS = 4;

DecoupledLookBackPrefixSum::DecoupledLookBackPrefixSum(bool validationLayer)
    : m_runtime(gpu::RuntimeCreateInfo{
          .validationLayer = validationLayer,
      }),
      m_commandPool(m_runtime, &m_runtime.computeQueue),
      m_deviceLocalMemory(m_runtime.deviceLocalMemory(),
                          (WEIGHT_COUNT * sizeof(unsigned int) * 2) * 1.5),
      m_hostVisibleMemory(m_runtime.hostVisibleMemory(),
                          WEIGHT_COUNT * sizeof(unsigned int) * 2 * 1.5),
      m_set0Layout(gpu::DescriptorSetLayout::builder()
                       .addBinding(0, vk::DescriptorType::eStorageBuffer,
                                   vk::ShaderStageFlagBits::eCompute)
                       .addBinding(1, vk::DescriptorType::eStorageBuffer,
                                   vk::ShaderStageFlagBits::eCompute)
                       .addBinding(2, vk::DescriptorType::eStorageBuffer,
                                   vk::ShaderStageFlagBits::eCompute)
                       .addBinding(3, vk::DescriptorType::eStorageBuffer,
                                   vk::ShaderStageFlagBits::eCompute)
                       .complete(m_runtime)),
      m_pipelineLayout(m_runtime, {&m_set0Layout}),
      m_computePipeline(
          m_runtime,
          "./shaders/parallel_decoupled_look_back_prefix_sum.cs.glsl",
          &m_pipelineLayout),
      m_descriptorPool(gpu::DescriptorPool::builder()
                           .require(1, &m_set0Layout)
                           .complete(m_runtime)),
      m_set0(m_descriptorPool.alloc(&m_set0Layout)), m_timer(m_runtime, 2),
      m_fence(m_runtime), m_transferFence(m_runtime) {}

DecoupledLookBackPrefixSum::~DecoupledLookBackPrefixSum() {}

std::vector<unsigned int> DecoupledLookBackPrefixSum::update() {
  std::vector<unsigned int> weights(WEIGHT_COUNT, 1u);
  size_t workGroupCount =
      (weights.size() + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE; // ceil int div
  m_commandPool.reset();
  m_fence.reset();
  m_deviceLocalMemory.reset();
  m_hostVisibleMemory.reset();

  std::pmr::monotonic_buffer_resource cpuBufferResouce{m_heap, sizeof(m_heap)};

  gpu::Buffer inputBuffer(m_runtime, weights.size() * sizeof(float),
                          vk::BufferUsageFlagBits::eStorageBuffer |
                              vk::BufferUsageFlagBits::eTransferDst |
                              vk::BufferUsageFlagBits::eTransferSrc,
                          &m_runtime.computeQueue, &m_deviceLocalMemory);
  gpu::Buffer prefixBuffer(m_runtime, weights.size() * sizeof(float),
                           vk::BufferUsageFlagBits::eStorageBuffer |
                               vk::BufferUsageFlagBits::eTransferDst |
                               vk::BufferUsageFlagBits::eTransferSrc,
                           &m_runtime.computeQueue, &m_deviceLocalMemory);

  gpu::Buffer partitionDescriptorBuffer(
      m_runtime, workGroupCount * (sizeof(float) + sizeof(float) + sizeof(int)),
      vk::BufferUsageFlagBits::eStorageBuffer |
          vk::BufferUsageFlagBits::eTransferDst |
          vk::BufferUsageFlagBits::eTransferSrc,
      &m_runtime.computeQueue, &m_deviceLocalMemory);

  gpu::Buffer atomicCounterBuffer(
      m_runtime, sizeof(unsigned int) + sizeof(unsigned int),
      vk::BufferUsageFlagBits::eStorageBuffer |
          vk::BufferUsageFlagBits::eTransferDst |
          vk::BufferUsageFlagBits::eTransferSrc,
      &m_runtime.computeQueue, &m_deviceLocalMemory);

  { // Staged upload
    gpu::Buffer stagingBuffer(m_runtime, weights.size() * sizeof(float),
                              vk::BufferUsageFlagBits::eTransferSrc,
                              &m_runtime.computeQueue, &m_hostVisibleMemory);
    m_transferFence.reset();
    stagingBuffer.upload<unsigned int>(weights);
    gpu::CommandBuffer cmd = m_commandPool.allocate();
    cmd.begin();
    cmd.copyBuffer(&inputBuffer, &stagingBuffer);
    cmd.end();
    m_runtime.computeQueue.submit()
        .commands(&cmd)
        .signal(&m_transferFence)
        .complete();
    m_transferFence.wait();
  }

  m_set0.write(0, inputBuffer, &m_set0Layout);
  m_set0.write(1, prefixBuffer, &m_set0Layout);
  m_set0.write(2, partitionDescriptorBuffer, &m_set0Layout);
  m_set0.write(3, atomicCounterBuffer, &m_set0Layout);

  gpu::CommandBuffer cmd = m_commandPool.allocate();
  cmd.begin();

  cmd.resetTimer(&m_timer);
  cmd.bindPipeline(&m_computePipeline, vk::PipelineBindPoint::eCompute);
  cmd.bindDescriptorSet(&m_set0, vk::PipelineBindPoint::eCompute,
                        &m_pipelineLayout);
  cmd.sampleTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, &m_timer);

  size_t a = weights.size();
  size_t b = WORK_GROUP_SIZE * ROWS;
  size_t groupCount = (a + b - 1) / b;
  std::cout << "dispatch: " << groupCount << std::endl;
  cmd.dispatch(groupCount);
  cmd.sampleTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, &m_timer);
  cmd.end();

  gpu::Fence computeComplete(m_runtime);

  m_runtime.computeQueue.submit()
      .commands(&cmd)
      .signal(&computeComplete)
      .complete();

  computeComplete.wait();

  std::vector<unsigned int> out;
  { // Staged download
    gpu::Buffer stagingBuffer(m_runtime, weights.size() * sizeof(float),
                              vk::BufferUsageFlagBits::eTransferDst,
                              &m_runtime.computeQueue, &m_hostVisibleMemory);
    m_transferFence.reset();
    gpu::CommandBuffer cmd = m_commandPool.allocate();
    cmd.begin();
    cmd.copyBuffer(&stagingBuffer, &prefixBuffer);
    cmd.end();
    m_runtime.computeQueue.submit()
        .commands(&cmd)
        .signal(&m_transferFence)
        .complete();
    m_transferFence.wait();
    out = stagingBuffer.download<unsigned int>();
  }

  auto results = m_timer.downloadResultDeltas(m_runtime);
  assert(results.size() == 1);
  m_duration = results[0];
  return out;
}
