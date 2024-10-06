#pragma once

#include <iostream>

namespace lout {

constexpr const char* VULKAN = "\x1B[100m";

std::ostream &info();
std::ostream &verbose();
std::ostream &warning();
std::ostream &error();

const char *endl();
const char *end();

}; // namespace log
