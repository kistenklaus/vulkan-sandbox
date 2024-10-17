#include <DecoupledLookBackPrefixSum.h>
#include <algorithm>
#include <chrono>
#include <gpu/buffer/Buffer.h>
#include <gpu/gpu.h>
#include <gpu/memory/memory.h>
#include <logging.h>
#include <ranges>
#include <vulkan/vulkan_enums.hpp>

int main() {

  DecoupledLookBackPrefixSum app = true;
  std::vector<float> in = {0, 0, 0, 0, 0};
  auto weightCount = DecoupledLookBackPrefixSum::WEIGHT_COUNT;
  for (size_t i = 0; i < 100000; i++) {
    auto prefix = app.update();
    float transferPerWeight = 2 * sizeof(float); // 8 byte
    float minTransfers = weightCount * transferPerWeight;
    /* assert(prefix.back() == weightCount - 1); */

    for(auto [i,v] : prefix | std::views::enumerate) {
      std::cout << "[" << i << "] = " << v << std::endl;
    }
    

    float max = std::ranges::max(prefix);
    std::cout << "max = " << max << std::endl;
    auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(app.duration());
    std::cout << "time: " << dur << std::endl;
    float seconds = dur.count();
    std::cout << "Elements Per Second: " << seconds / weightCount <<  std::endl;
    std::cout << "Speed:" << minTransfers / (seconds * 1e9) << "Gb/s (target=500Gb/s)"<< std::endl;
    break;
  }

  return 1;
}
