#pragma once
#include "gpu/Runtime.fwd.h"
#include "gpu/cmd/CommandBuffer.h"
#include "gpu/sync/Fence.h"
#include "gpu/sync/Semaphore.h"
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace gpu {

template <typename Allocator> class QueueSubmission;

class Queue {
public:
  Queue() = default;
  Queue(const Runtime &runtime, uint32_t familyIndex, uint32_t index);
  Queue(vk::Device device, uint32_t familyIndex, uint32_t index);
  ~Queue() = default;
  Queue(const Queue &) noexcept = default;
  Queue &operator=(const Queue &) noexcept = default;
  Queue(Queue &&) noexcept = default;
  Queue &operator=(Queue &&) noexcept = default;

  inline vk::Queue handle() const { return m_queue; }

  template <typename Allocator = std::allocator<void *>>
  QueueSubmission<Allocator>
  submit(size_t commamndBufferCountHint = 1, size_t waitSemCountHint = 1,
         size_t signalSemCountHint = 1, const Allocator &alloc = {});

  inline uint32_t familyIndex() const { return m_familyIndex; }

private:
  uint32_t m_familyIndex;
  vk::Queue m_queue;
};

template <typename Allocator = std::allocator<void *>> class QueueSubmission {
  using AllocatorTraits = std::allocator_traits<Allocator>;
  using CommandBufferAllocator =
      AllocatorTraits::template rebind_alloc<vk::CommandBuffer>;
  using SemaphoreAllocator =
      AllocatorTraits::template rebind_alloc<vk::Semaphore>;

public:
  QueueSubmission(vk::Queue queue, size_t commandBufferCountHint,
                  size_t waitSemCountHint, size_t signalSemCountHint,
                  const Allocator &alloc = {})
      : m_queue(queue), m_commandBuffers(alloc), m_waitSemaphores(alloc),
        m_signalSemaphores(alloc), m_fence(std::nullopt) {
    m_commandBuffers.reserve(commandBufferCountHint);
    m_waitSemaphores.reserve(waitSemCountHint);
    m_signalSemaphores.reserve(signalSemCountHint);
  }

  void complete() {
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(m_commandBuffers);
    submitInfo.setWaitSemaphores(m_waitSemaphores);
    submitInfo.setSignalSemaphores(m_signalSemaphores);
    vk::Result rSubmit = m_queue.submit(
        1, &submitInfo, m_fence.has_value() ? *m_fence : VK_NULL_HANDLE);
    if (rSubmit != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to submit to queue");
    }
  }

  QueueSubmission &commands(const CommandBuffer *commandBuffer) {
    m_commandBuffers.push_back(commandBuffer->handle());
    return *this;
  }

  QueueSubmission &waitFor(const Semaphore *wait) {
    m_waitSemaphores.push_back(wait->handle());
    return *this;
  }

  QueueSubmission &signal(const Semaphore *signal) {
    m_signalSemaphores.push_back(signal);
    return *this;
  }

  QueueSubmission &signal(const Fence *fence) {
    assert(!m_fence.has_value());
    m_fence = fence->handle();
    return *this;
  }

private:
  vk::Queue m_queue;
  std::vector<vk::CommandBuffer, CommandBufferAllocator> m_commandBuffers;
  std::vector<vk::Semaphore, SemaphoreAllocator> m_waitSemaphores;
  std::vector<vk::Semaphore, SemaphoreAllocator> m_signalSemaphores;
  std::optional<vk::Fence> m_fence;
};

template <typename Allocator>
QueueSubmission<Allocator>
Queue::submit(size_t commandBufferCountHint, size_t waitSemCountHint,
              size_t signalSemCountHint, const Allocator &alloc) {
  return {m_queue, commandBufferCountHint, waitSemCountHint, signalSemCountHint,
          alloc};
}

} // namespace gpu
