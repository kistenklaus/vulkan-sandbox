#include "./Queue.h"
#include "Runtime.h"

gpu::Queue::Queue(const Runtime &runtime, uint32_t queueFamilyIndex,
                  uint32_t queueIndex)
    : m_familyIndex(queueFamilyIndex) {
  m_queue = runtime.device.getQueue(queueFamilyIndex, queueIndex);
}

gpu::Queue::Queue(vk::Device device, uint32_t queueFamilyIndex,
                  uint32_t queueIndex)
    : m_familyIndex(queueFamilyIndex) {
  m_queue = device.getQueue(queueFamilyIndex, queueIndex);
}
