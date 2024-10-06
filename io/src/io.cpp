#include "./io.h"
#include <fstream>

std::vector<std::byte> io::readFileSync(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to read file " + filepath);
  }
  std::ios::pos_type fileSize = file.tellg();
  std::vector<std::byte> buffer(static_cast<size_t>(fileSize));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
  file.close();
  return buffer;
}

