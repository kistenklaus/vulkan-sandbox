#pragma once

#include <cstddef>
#include <cstdlib>
#include <unistd.h>

namespace wie {

template<typename T = void>
struct PageAllocator {
public:
  template<typename U>
  struct rebind {
    using other = PageAllocator<U>;
  };
  using value_type = T;

  T* allocate(size_t n) {
    return reinterpret_cast<T*>(aligned_alloc(sysconf(_SC_PAGESIZE), n));
  }

  void deallocate(value_type* p, size_t n){
    free(p);
  }
  
private:
};

} // namespace wie
