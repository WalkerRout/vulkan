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

// Main Application Pipeline
public:
  auto run() -> void;

private:
  auto init_window(void) -> void;
  auto init_vulkan(void) -> void;
  auto main_loop(void) -> void;
  auto cleanup(void) -> void;
// End of Main Application Pipeline

// Utility
private:
  auto create_instance(void) -> void;
  auto setup_debug_messenger(void) -> void;
  auto create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_msnger) -> VkResult;
  auto destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_msnger, const VkAllocationCallbacks* p_allocator) -> void;

public:
  auto get_window_user_ptr(void) const -> void*;
// End of Utility

private:
  GLFWwindow* window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
};

} // end of namespace triangle

#endif // TRIANGLE_H