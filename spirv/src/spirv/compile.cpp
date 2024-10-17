#include "./compile.h"
#include <logging.h>
#include <shaderc/env.h>
#include <shaderc/shaderc.h>
#include <shaderc/shaderc.hpp>
#include <shaderc/status.h>
#include <vulkan/vulkan_core.h>

std::vector<uint32_t> spirv::compileShader(const std::vector<std::byte> &source,
                                           const std::string &filename) {

  static shaderc::Compiler compiler;
  const std::string x(reinterpret_cast<const char *>(source.data()),
                      source.size());

  shaderc::CompileOptions options;
  options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
  options.SetSourceLanguage(shaderc_source_language_glsl);
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
      reinterpret_cast<const char *>(source.data()), source.size(),
      shaderc_shader_kind::shaderc_compute_shader, filename.c_str(),
      options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    lout::error() << "Failed to compile Shader: " << filename << '\n'
                  << result.GetErrorMessage() << lout::endl();
    throw std::runtime_error("Failed to compile Shader");
  }
  return {result.begin(), result.end()};
}
