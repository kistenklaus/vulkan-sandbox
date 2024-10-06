#pragma once

#include "gpu/memory/Resource.h"
#include <limits>
namespace gpu::memory {

class MonotonicResource : public Resource {
public:
  MonotonicResource(Resource *upstream, size_t capacity,
                    size_t alignment = std::numeric_limits<size_t>::max())
      : m_capacity(capacity), m_topOfStack(0),
        m_memory(upstream->alloc(capacity, alignment)), m_upstream(upstream) {}

  ~MonotonicResource() override {
    if (m_memory) {
      m_upstream->free(m_memory);
    }
  }

  Ptr alloc(size_t size, size_t alignment) override {
    assert(alignment != std::numeric_limits<size_t>::max());

    Ptr ptr{
        .deviceMemory = m_memory.deviceMemory,
        .offset = static_cast<vk::DeviceSize>(m_topOfStack +
                                              m_topOfStack % alignment),
    };
    m_topOfStack = ptr.offset + size;
    assert(m_topOfStack < m_capacity);
    return ptr;
  }
  void free(Ptr ptr) override {
    // NO OP!
  }

  uint32_t memoryTypeIndex() const override {
    return m_upstream->memoryTypeIndex();
  }

private:
  size_t m_capacity;
  size_t m_topOfStack;
  Ptr m_memory;
  Resource *m_upstream;
};

} // namespace gpu::memory
