
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "triangle.h"

namespace triangle {

//TriangleApplication::TriangleApplication() {
//  run();
//}

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
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Triangle Application";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;
  createInfo.enabledLayerCount = 0;

  if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create instance");
}

}