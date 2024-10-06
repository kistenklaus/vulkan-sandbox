#pragma once

#include <cstring>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace gpu {

template <typename HostAllocator = std::allocator<uint64_t>,
          bool Atomic = false>
class PipelineDescription {
private:
  using AllocatorTraits = std::allocator_traits<HostAllocator>;

  using HostLayoutAllocator =
      AllocatorTraits::template rebind_alloc<vk::DescriptorSetLayout>;
  using HostLayoutAllocatorTraits = std::allocator_traits<HostLayoutAllocator>;
  using LayoutPtr = typename HostLayoutAllocatorTraits::pointer;

  using HostPoolSizeAllocator =
      AllocatorTraits::template rebind_alloc<vk::DescriptorPoolSize>;
  using HostPoolSizeAllocatorTraits =
      std::allocator_traits<HostPoolSizeAllocator>;
  using PoolSizePtr = typename HostPoolSizeAllocatorTraits::pointer;

  using ReferenceCountBase = size_t;
  using ReferenceCount =
      std::conditional_t<Atomic, std::atomic<ReferenceCountBase>,
                         ReferenceCountBase>;

public:
  PipelineDescription(std::span<vk::DescriptorSetLayout> layouts,
                      std::span<vk::DescriptorPoolSize> poolSizes,
                      const HostAllocator &alloc = {})
      : m_layoutCount(layouts.size()), m_layoutAllocator(alloc),
        m_poolSizeCount(poolSizes.size()), m_poolSizeAllocator(alloc),
        m_refCount(1) {
    m_layouts =
        HostLayoutAllocatorTraits::allocate(m_layoutAllocator, m_layoutCount);
    std::memcpy(m_layouts, layouts.data(),
                sizeof(vk::DescriptorSetLayout) * m_layoutCount);
    m_poolSizes = HostPoolSizeAllocatorTraits::allocate(m_poolSizeAllocator,
                                                        m_poolSizeCount);
    std::memcpy(m_poolSizes, poolSizes.data(),
                sizeof(vk::DescriptorPoolSize) * m_poolSizeCount);
  }

  inline const std::span<const vk::DescriptorSetLayout> layouts() const {
    return {m_layouts, m_layoutCount};
  }

  inline const std::span<const vk::DescriptorPoolSize> poolSizes() const {
    return {m_poolSizes, m_poolSizeCount};
  }

  ~PipelineDescription() {
    const ReferenceCountBase prev = m_refCount--;
    if (prev == 1) {
      m_layoutAllocator.deallocate(m_layouts, m_layoutCount);
      m_poolSizeAllocator.deallocate(m_poolSizes, m_poolSizeCount);
    }
  }
  PipelineDescription(const PipelineDescription &o) noexcept
      : m_layoutCount(o.m_layoutCount), m_layouts(o.m_layouts),
        m_layoutAllocator(
            HostLayoutAllocatorTraits::select_on_copy_construction(
                o.m_layoutAllocator)),
        m_poolSizeCount(o.m_poolSizeCount), 
        m_poolSizes(o.m_poolSizes),
        m_poolSizeAllocator(HostLayoutAllocatorTraits::select_on_copy_costruction(o.m_layoutAllocator)),
        m_refCount(o.m_refCount) {}

private:
  const size_t m_layoutCount;
  LayoutPtr m_layouts;
  [[no_unique_address]] HostLayoutAllocator m_layoutAllocator;

  size_t m_poolSizeCount;
  PoolSizePtr m_poolSizes;
  [[no_unique_address]] HostPoolSizeAllocator m_poolSizeAllocator;

  ReferenceCount m_refCount;

  void foo() {}
};

namespace pmr {
using PipelineDescription =
    PipelineDescription<std::pmr::polymorphic_allocator<uint64_t>>;
}

} // namespace gpu
