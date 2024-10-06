#pragma once

#include <cstdint>
#include <string>
#include <vector>
namespace spirv {

std::vector<uint32_t> compileShader(const std::vector<std::byte>& source, const std::string& filename);
}
