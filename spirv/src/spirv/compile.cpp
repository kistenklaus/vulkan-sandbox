#include "./compile.h"
#include <shaderc/shaderc.h>
#include <shaderc/shaderc.hpp>
#include <shaderc/status.h>

std::vector<uint32_t> spirv::compileShader(const std::vector<std::byte> &source,
                                           const std::string &filename) {

  static shaderc::Compiler compiler;
  const std::string x(reinterpret_cast<const char *>(source.data()),
                      source.size());

  shaderc::CompileOptions options;
  options.SetSourceLanguage(shaderc_source_language_glsl);
  options.SetOptimizationLevel(shaderc_optimization_level_zero);
  shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
      reinterpret_cast<const char *>(source.data()), source.size(),
      shaderc_shader_kind::shaderc_glsl_compute_shader, filename.c_str(),
      options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    /* std::cout << colorful::ERROR << "Failed to compile Shader:\n" */
    /*           << result.GetErrorMessage() << colorful::RESET << std::endl; */
    throw std::runtime_error("Failed to compile Shader");
  }
  return {result.begin(), result.end()};
}
