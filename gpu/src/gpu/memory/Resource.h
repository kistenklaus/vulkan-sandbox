#pragma once

#include "gpu/memory/Ptr.h"

namespace gpu::memory {

class Resource {
  public:
    Resource() = default;
    virtual ~Resource() = default;
    
    virtual Ptr alloc(size_t size, size_t alignment) = 0;
    virtual void free(Ptr ptr) = 0;
    virtual uint32_t memoryTypeIndex() const = 0;
};

};
