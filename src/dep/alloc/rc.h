#pragma once
#include <memory>
#include <strings.h>
#include <type_traits>

namespace wie {

template <typename T, typename Allocator = std::allocator<T>> struct Rc {
public:
  using RefCountType = size_t;
  using ValueAllocator = Allocator;
  using ValueAllocTraits = std::allocator_traits<ValueAllocator>;
  using RefAllocator =
      typename ValueAllocTraits::template rebind_alloc<RefCountType>;
  using RefAllocatorTraits = std::allocator_traits<RefAllocator>;

  // constructor
  explicit Rc(T *p)
      : m_valueAllocator{}, m_refAllocator{}, m_value(p),
        m_refCount(m_refAllocator.allocate(1)) {
    *m_refCount = 1;
  }

  explicit Rc(const Allocator &allocator, T *p)
      : m_valueAllocator(allocator), m_refAllocator{allocator}, m_value(p),
        m_refCount(m_refAllocator.allocate(1)) {
    *m_refCount = 1;
  }
  // make constructors
  template <typename... Args> static Rc<T, Allocator> make(Args &&...args) {
    Allocator alloc = Allocator();
    T *value = alloc.allocate(1);
    ValueAllocTraits::construct(alloc, value, std::forward<Args>(args)...);
    return Rc(alloc, value);
  }
  template <typename... Args> static Rc<T, Allocator> make(const Allocator& alloc, Args &&...args) {
    T *value = alloc.allocate(1);
    ValueAllocTraits::construct(alloc, value, std::forward<Args>(args)...);
    return Rc(alloc, value);
  }

  // destructor
  ~Rc() {
    m_refCount -= 1;
    if (m_refCount == 0) {
      m_refAllocator.deallocate(m_refCount, 1);
      m_valueAllocator.deallocate(m_value, 1);
    }
  }
  // copy constructor
  Rc(const Rc &other) noexcept
      : m_valueAllocator(other.m_valueAllocator),
        m_refAllocator(other.m_refAllocator), m_value(other.m_value),
        m_refCount(other.m_refCount) {
    m_refCount += 1;
  }
  // move constructor
  Rc(Rc &&other) noexcept
      : m_valueAllocator(std::move(other.m_valueAllocator)),
        m_refAllocator(std::move(other.m_refAllocator)), m_value(other.m_value),
        m_refCount(other.m_refCount) {}
  // copy assignment
  Rc &operator=(const Rc &other) noexcept {
    if (&other == this) {
      if constexpr (std::is_same_v<typename ValueAllocTraits::
                                       propagate_on_container_copy_assignment,
                                   std::true_type>()) {
        assert(m_valueAllocator == other.m_valueAllocator);
        m_valueAllocator = other.m_valueAllocator;
      }
      if constexpr (std::is_same_v<typename ValueAllocTraits::
                                       propagate_on_container_copy_assignment,
                                   std::true_type>()) {
        assert(m_refAllocator == other.m_refAllocator);
        m_refAllocator = other.m_refAllocator;
      }
      m_refCount = other.m_refCount + 1;
      m_value = other.m_value;
    }
    return *this;
  }
  // move assignment
  Rc &operator=(Rc &&other) noexcept {
    if (&other == this) {
      if constexpr (std::is_same_v<typename ValueAllocTraits::
                                       propagate_on_container_move_assignment,
                                   std::true_type>()) {
        assert(m_valueAllocator == other.m_valueAllocator);
        m_valueAllocator = other.m_valueAllocator;
      }
      if constexpr (std::is_same_v<typename ValueAllocTraits::
                                       propagate_on_container_move_assignment,
                                   std::true_type>()) {
        assert(m_refAllocator == other.m_refAllocator);
        m_refAllocator = other.m_refAllocator;
      }
      m_refCount = other.m_refCount;
      m_value = other.m_value;
    }
    return *this;
  }

  // operators
  T *operator->() { return m_value; }
  const T *operator->() const { return m_value; }

  T &operator*() { return *m_value; }
  const T &operator*() const { return *m_value; }

  T* get() {
    return m_value;
  }
  const T* get() const {
    return m_value;
  }

private:
  Allocator m_valueAllocator;
  RefAllocator m_refAllocator;
  T *m_value;
  size_t *m_refCount;
};

} // namespace wie
