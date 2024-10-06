#pragma once

#include "gpu/memory/Resource.h"
namespace gpu::memory {

class PolymorphicAllocator {
public:
  PolymorphicAllocator(Resource *resource) : m_resource(resource) {}

  inline Ptr alloc(size_t size, size_t alignemnt) {
    return m_resource->alloc(size, alignemnt);
  }

  inline void free(Ptr ptr) { m_resource->free(ptr); }

  uint32_t memoryTypeIndex() const {return m_resource->memoryTypeIndex();}

private:
  Resource *m_resource;
};
} // namespace gpu::memory
