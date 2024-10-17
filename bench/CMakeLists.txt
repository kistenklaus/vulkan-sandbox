# Disable the Google Benchmark requirement on Google Test
set(BENCHMARK_ENABLE_TESTING OFF)

include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG origin/main
)

FetchContent_MakeAvailable(googlebenchmark)

set(CMAKE_CXX_STANDARD 26)

set(BENCH_NAME ${PROJECT_NAME}-bench)
add_executable(${BENCH_NAME} 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bench.cpp
)

target_compile_options(${BENCH_NAME} PUBLIC -O2)

target_link_libraries(${BENCH_NAME} benchmark::benchmark)
target_link_libraries(${BENCH_NAME} ${PROJECT_NAME})

# Link Shlwapi to the project
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
  target_link_libraries(${BENCH_NAME} Shlwapi)
endif()
