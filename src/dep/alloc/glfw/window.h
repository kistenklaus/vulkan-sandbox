#pragma once
#include "glfw/glfw.h"
#include <tuple>

namespace glfw::window {

extern GLFWwindow* g_window;

void createWindow();

bool shouldClose();

void pollEvents();

void destroyWindow();

std::tuple<uint32_t, uint32_t> getFramebufferSize();
}
