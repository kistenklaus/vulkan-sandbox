#pragma once

#include <benchmark/benchmark.h>
#include <chrono>
#include <concepts>
#include <gpu/time/Timer.h>
#include <optional>
#include <type_traits>

template <typename App>
concept app = requires(App app) {
  { app.update() };
  {
    app.duration()
  } -> std::convertible_to<gpu::time::Clock::duration>;
};

template <typename App>
  requires app<App>
class AppFixture : public benchmark::Fixture {
public:
  void SetUp(::benchmark::State &) override { 
    app.emplace();
  }
  void TearDown(::benchmark::State &state) override { 
    app.reset();
  }
protected:
  std::optional<App> app = std::nullopt;
};
