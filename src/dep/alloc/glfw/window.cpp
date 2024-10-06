#include "./window.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

GLFWwindow *glfw::window::g_window = nullptr;

void glfw::window::createWindow() {
  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("Failed to initalize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  g_window = glfwCreateWindow(800, 600, "FLOATING", nullptr, nullptr);
  if (g_window == nullptr) {
    throw std::runtime_error("Failed to create GLFW Window");
  }
}

bool glfw::window::shouldClose() { return glfwWindowShouldClose(g_window); }

void glfw::window::pollEvents() { return glfwPollEvents(); }

void glfw::window::destroyWindow() {
  glfwDestroyWindow(g_window);
  glfwTerminate();
}

std::tuple<uint32_t, uint32_t> glfw::window::getFramebufferSize() {
  int width, height;
  glfwGetFramebufferSize(glfw::window::g_window, &width, &height);
  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

