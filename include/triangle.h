#ifndef TRIANGLE_H
#define TRIANGLE_H

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

namespace triangle {

struct TriangleApplication {
  static const std::size_t WIDTH  = 800;
  static const std::size_t HEIGHT = 600;

  //TriangleApplication();

public:
  auto run() -> void;

private:
  // Main Application Pipeline
  auto init_window(void) -> void;
  auto init_vulkan(void) -> void;
  auto main_loop(void) -> void;
  auto cleanup(void) -> void;
  // End of Main Application Pipeline

  auto create_instance(void) -> void;

private:
  GLFWwindow* window;
  VkInstance instance;
};

}

#endif // TRIANGLE_H