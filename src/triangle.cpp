
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

#include "triangle.h"

static auto check_validation_layer_support(void) -> bool;
static auto message_callback_get_required_extensions(void) -> std::vector<const char*>;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
  VkDebugUtilsMessageTypeFlagsEXT message_type, 
  const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
  void* p_user_data);

namespace triangle {

auto TriangleApplication::run(void) -> void {
  init_window();
  init_vulkan();
  main_loop();
  cleanup();
}

auto TriangleApplication::init_window(void) -> void {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "TriangleApplication", nullptr, nullptr);
  if(window == nullptr)
    throw std::runtime_error("Error - (window) is nullptr");
}

auto TriangleApplication::init_vulkan(void) -> void {
  create_instance();
}

auto TriangleApplication::main_loop(void) -> void {
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

auto TriangleApplication::cleanup(void) -> void {
  // vulkan cleanup
  vkDestroyInstance(instance, nullptr);

  // glfw cleanup
  glfwDestroyWindow(window);
  glfwTerminate();
}
// End of Main Application Pipeline

auto TriangleApplication::create_instance(void) -> void {
  if(enable_validation_layers && !check_validation_layer_support())
    throw std::runtime_error("Error - validation layers requested, but unavailable");

  VkApplicationInfo app_info;
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Triangle Application";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  if(enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledExtensionNames = validation_layers.data();
  } else {
    create_info.enabledLayerCount = 0;
  }

  auto extensions = message_callback_get_required_extensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();
  create_info.enabledLayerCount = 0;

  if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create instance");
}

} // end of namespace triangle

static auto check_validation_layer_support(void) -> bool {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  // try const auto& or const auto* maybe
  for(const char* layer_name : triangle::validation_layers) {
    auto layer_found = false;

    for(const auto& layer_properties : available_layers) {
      if(strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if(!layer_found) {
      return false;
    }
  }

  return true;
}

static auto message_callback_get_required_extensions(void) -> std::vector<const char*> {
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if(triangle::enable_validation_layers)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

// first parameter is one of the following:
//
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 
// - Diagnostic message
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: 
// - Informational message like the creation of a resource
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: 
// - Message about behavior that is not necessarily an error, but very likely a 
//   bug in your application
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
// - Message about behavior that is invalid and may cause crashes
//
// second parameter is one of the following:
//
// VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
// - Some event has happened that is unrelated to the specification or performance
// VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: 
// - Something has happened that violates the specification or indicates a
//   possible mistake
// VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
// - Potential non-optimal use of Vulkan
//
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
  VkDebugUtilsMessageTypeFlagsEXT message_type, 
  const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
  void* p_user_data) {
  std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;
  return VK_FALSE;
}