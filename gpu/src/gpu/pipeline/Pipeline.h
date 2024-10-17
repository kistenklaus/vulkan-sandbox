#pragma once

#include <vulkan/vulkan.hpp>

namespace gpu {
class Pipeline {
public:
  virtual ~Pipeline() = default;

  inline vk::Pipeline handle() const { return m_pipeline; }

protected:
  Pipeline() = default;

  Pipeline(vk::Pipeline pipeline) : m_pipeline(pipeline) {}

  vk::Pipeline m_pipeline;
};

} // namespace gpu
