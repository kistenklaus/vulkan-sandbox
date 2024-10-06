#include "./logging.h"

using namespace lout;


constexpr const char* RST = "\x1B[0m";
constexpr const char* RED_TEXT = "\x1B[31m";
constexpr const char* GREEN_TEXT = "\x1B[32m";
constexpr const char* YELLOW_TEXT = "\x1B[33m";
constexpr const char* BLUE_TEXT = "\x1B[34m";
constexpr const char* DARK_GRAY_BACKGROUND = "\x1B[100m";

static std::ostream info_ostream(std::cout.rdbuf());
std::ostream &lout::info() {
  return info_ostream << BLUE_TEXT;
}

static std::ostream verbose_ostream(std::cout.rdbuf());

std::ostream &lout::verbose() {
  return verbose_ostream << GREEN_TEXT;
}

static std::ostream warning_ostream(std::cout.rdbuf());
std::ostream &lout::warning() {
  return verbose_ostream << YELLOW_TEXT;
}

static std::ostream error_ostream(std::cout.rdbuf());
std::ostream &lout::error() {
  return verbose_ostream << RED_TEXT;
}

const char *lout::endl() { return "\x1B[0m\n"; }

const char *lout::end() { return "\x1B[0m"; }

