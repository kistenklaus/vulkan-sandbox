#include "SubgroupPrefixSum.h"
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

  using App = DecoupledLookBackPrefixSum;
  App app = true;
  auto weightCount = App::WEIGHT_COUNT;
  for (size_t i = 0; i < 100000; i++) {
    auto prefix = app.update();
    float transferPerWeight = 2 * sizeof(float); // 8 byte
    float minTransfers = weightCount * transferPerWeight;

    if constexpr (false) {
      for (auto [i, v] : prefix | std::views::enumerate) {
        std::cout << "[" << i << "] = " << v << std::endl;
        if (v == 0) {
          std::cout << "..." << std::endl;
          break;
        }
      }
    }
    std::cout << "last: " << prefix.back() << std::endl;
    /* assert(prefix.back() == weightCount); */

    /* float max = std::ranges::max(prefix); */
    /* std::cout << "max = " << max << std::endl; */
    auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(
        app.duration());
    std::cout << "time: "
              << std::chrono::duration_cast<
                     std::chrono::duration<double, std::milli>>(dur)
              << std::endl;
    float seconds = dur.count();
    float speed = minTransfers / (seconds * 1e9);
    std::cout << "Elements Per Second: " << seconds / weightCount << std::endl;
    std::cout << "Speed:" << speed << "Gb/s (target=500Gb/s) efficiency=("
              << (speed / 504) * 100 << "%)" << std::endl;
    break;
  }

  return 1;
}
