#pragma once

namespace colorful {

namespace __flags {
constexpr const char* RST = "\x1B[0m";
constexpr const char* RED_TEXT = "\x1B[31m";
constexpr const char* GREEN_TEXT = "\x1B[32m";
constexpr const char* YELLOW_TEXT = "\x1B[33m";
constexpr const char* BLUE_TEXT = "\x1B[34m";
constexpr const char* DARK_GRAY_BACKGROUND = "\x1B[100m";
}

constexpr const char* VULKAN = __flags::DARK_GRAY_BACKGROUND;
constexpr const char* VERBOSE = __flags::GREEN_TEXT;
constexpr const char* INFO = __flags::BLUE_TEXT; 
constexpr const char* WARNING = __flags::YELLOW_TEXT;
constexpr const char* ERROR = __flags::RED_TEXT;
constexpr const char* RESET = __flags::RST;

}


