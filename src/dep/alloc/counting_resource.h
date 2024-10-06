#pragma once

#include <memory_resource>

struct CountingMemoryResource : public std::pmr::memory_resource {
public:
  CountingMemoryResource() : m_upstream(std::pmr::get_default_resource()) {}
  CountingMemoryResource(std::pmr::memory_resource *upstream)
      : m_upstream(upstream) {}
  CountingMemoryResource(const CountingMemoryResource &) = default;

  virtual void *do_allocate(std::size_t bytes, std::size_t alignment) override {
    m_allocCount += 1;
    return m_upstream->allocate(bytes, alignment);
  }

  virtual void do_deallocate(void *p, std::size_t bytes,
                             std::size_t alignment) override {
    return m_upstream->deallocate(p, bytes, alignment);
  }

  virtual bool
  do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
    if (const CountingMemoryResource *d =
            dynamic_cast<const CountingMemoryResource *>(&other);
        d != nullptr) {
      return d->m_upstream != m_upstream;
    } else {
      return false;
    }
  }

  inline size_t allocCount() const { return m_allocCount; }

private:
  uint32_t m_allocCount = 0;
  std::pmr::memory_resource *m_upstream;
};
