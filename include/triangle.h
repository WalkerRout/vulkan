#ifndef TRIANGLE_H
#define TRIANGLE_H

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

#define ENABLE_VALIDATION_LAYERS // enabled by default

namespace triangle {

const std::vector<const char*> validation_layers = {
  "VK_LAYER_LUNARG_standard_validation"
  //"VK_LAYER_KHRONOS_validation"
};

#ifdef ENABLE_VALIDATION_LAYERS
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif // NDEBUG

struct TriangleApplication {
  static const std::size_t WIDTH  = 800;
  static const std::size_t HEIGHT = 600;

  TriangleApplication() = default;

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

} // end of namespace triangle

#endif // TRIANGLE_H