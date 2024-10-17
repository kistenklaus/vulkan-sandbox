#pragma once
#include "gpu/Runtime.fwd.h"
#include <chrono>
#include <memory>
#include <shaderc/shaderc.hpp>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace gpu {

namespace time {
class Clock {
public:
  using rep = uint64_t;
  using period = std::nano;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<class Clock>;
  static constexpr bool is_steady = true;
  static time_point now() { assert(false); }
};
}; // namespace time

class Timer {
public:
  Timer(const Runtime &runtime, size_t samples);
  ~Timer() { m_device.destroyQueryPool(m_queryPool); }
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;
  Timer(Timer &&o) noexcept
      : m_device(std::move(o.m_device)),
        m_queryCount(std::move(o.m_queryCount)),
        m_nextQuery(std::move(o.m_nextQuery)),
        m_queryPool(std::move(o.m_queryPool)) {}
  Timer &operator=(Timer &&o) noexcept {
    if (this == &o) {
      return *this;
    }
    this->~Timer();
    std::swap(m_device, o.m_device);
    std::swap(m_queryCount, o.m_queryCount);
    std::swap(m_nextQuery, o.m_nextQuery);
    std::swap(m_queryPool, o.m_queryPool);
    return *this;
  }

  inline vk::QueryPool handle() const { return m_queryPool; }

  uint32_t nextQuery() { return m_nextQuery++; }

  inline uint32_t queryCount() const { return m_queryCount; }

  void reset() { m_nextQuery = 0; }

  template <typename TimePointAllocator =
                std::allocator<time::Clock::time_point>>
  std::vector<time::Clock::time_point, TimePointAllocator>
  downloadResultTimestamps(const Runtime &runtime, const TimePointAllocator &alloc = {}) {
    std::vector<time::Clock::time_point, TimePointAllocator> results(m_nextQuery, alloc);
    vk::Result rQueryPoolResults = m_device.getQueryPoolResults(
        m_queryPool, 0, m_nextQuery, sizeof(uint64_t) * m_queryCount,
        results.data(), sizeof(uint64_t),
        vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64);

    if (rQueryPoolResults != vk::Result::eSuccess) {
      throw std::runtime_error(
          "Failed to download query pool results for timer");
    }
    return results;
  }

  template <typename DurationAllocator =
                std::allocator<time::Clock::duration>>
  std::vector<time::Clock::duration, DurationAllocator>
  downloadResultDeltas(const Runtime &runtime, const DurationAllocator &alloc = {}) {
    using TimePointAllocator = std::allocator_traits<DurationAllocator>::template rebind_alloc<time::Clock::time_point>;
    auto timestamps = downloadResultTimestamps<TimePointAllocator>(runtime, alloc);
    if (timestamps.size() <= 1){
      return {};
    }
    std::vector<time::Clock::duration, DurationAllocator> deltas(timestamps.size() -1, alloc);
    for(size_t i = 0; i < timestamps.size() - 1; ++i) {
      deltas[i] = timestamps[i + 1] - timestamps[i];
    }
    return deltas;
  }

private:
  vk::Device m_device;
  uint32_t m_queryCount;
  uint32_t m_nextQuery = 0;
  vk::QueryPool m_queryPool;
  float m_timestampPeriod;
};

} // namespace gpu
