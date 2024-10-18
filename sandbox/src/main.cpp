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
  App app = false;
  auto weightCount = App::WEIGHT_COUNT;
  auto prefix = app.update();
  float transferPerWeight = 2 * sizeof(float); // 8 byte
  float minTransfers = weightCount * transferPerWeight;

  if (weightCount <= 512 * 64) {
    float prev = 0.0f;
    bool dotted = false;
    for (auto [i, v] : prefix | std::views::enumerate) {
      if constexpr (true) {
        if (v == prev) {
          if (!dotted) {
            dotted = true;
            std::cout << "...\n";
          }
        } else {
          dotted = false;
          prev = v;
          std::cout << "[" << i << "] = " << v << '\n';
        }
      } else {
        std::cout << "[" << i << "] = " << v << '\n';
      }
    }
  }
  std::cout << "last: " << prefix.back() << std::endl;

  if (weightCount <= 64e5) {
    float max = std::ranges::max(prefix);
    std::cout << "max: " << max << std::endl;
  }

  auto dur =
      std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(
          app.duration());

  std::cout
      << "time: "
      << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
             dur)
      << " (target=2ms)" << std::endl;
  float seconds = dur.count();
  std::cout << seconds << std::endl;
  float speed = minTransfers / (seconds * 1e9);
  std::cout << "InputSize: " << weightCount << std::endl;
  std::cout << "Elements Per Second: " << weightCount / seconds << std::endl;
  std::cout << "Speed:" << speed << "Gb/s (target=500Gb/s) efficiency=("
            << (speed / 504) * 100 << "%)" << std::endl;

  assert(prefix.back() == weightCount);

  return 1;
}
