#pragma once
#include "rc.h"
#include <cstddef>
#include <memory>

namespace wie {

namespace internal {
struct CountingAllocatorStorage {
  size_t m_allocCount;
  size_t m_deallocCount;

  CountingAllocatorStorage() : m_allocCount(0), m_deallocCount(0) {}
};

} // namespace internal

template <typename Allocator = std::allocator<void>> struct CountingAllocator {
  using alloc_traits = std::allocator_traits<Allocator>;
  using value_type = typename alloc_traits::value_type;
  using pointer = typename alloc_traits::pointer;
  using size_type = typename alloc_traits::size_type;

  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using is_always_equals = std::false_type;

  CountingAllocator()
      : m_parentAllocator{},
        m_storage(wie::Rc<internal::CountingAllocatorStorage>::make()) {}

  CountingAllocator(const Allocator &parentAllocator)
      : m_parentAllocator(parentAllocator),
        m_storage(wie::Rc<internal::CountingAllocatorStorage>::make()) {}
  ~CountingAllocator() = default;

  pointer allocate(size_t n) {
    m_storage->m_allocCount += 1;
    return m_parentAllocator.allocate(n);
  }

  void deallocate(pointer p, size_t n) {
    m_storage->m_deallocCount += 1;
    m_parentAllocator.deallocate(p, n);
  }

  size_t allocCount() const { return m_storage->m_allocCount; }

  size_t deallocCount() const { return m_storage->m_deallocCount; }

  template <typename OtherAlloc>
  bool operator==(const CountingAllocator<OtherAlloc> &other) const {
    return m_storage.get() == other.m_storage.get();
  }

  template <typename OtherAlloc>
  bool operator!=(const CountingAllocator<OtherAlloc> &other) const {
    return m_storage.get() != other.m_storage.get();
  }

private:
  Allocator m_parentAllocator;
  Rc<internal::CountingAllocatorStorage> m_storage;
};

} // namespace wie
