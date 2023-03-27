#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

struct TriangleApplication {
  static const std::size_t WIDTH  = 800;
  static const std::size_t HEIGHT = 600;

public:
  auto run() -> void {
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
  }

private:
  auto init_window(void) -> void {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "TriangleApplication", nullptr, nullptr);
    if(window == nullptr)
      throw std::runtime_error("Error - (window) is nullptr");
  }

  auto init_vulkan(void) -> void {

  }

  auto main_loop(void) -> void {
    while(!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  auto cleanup(void) -> void {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

private:
  GLFWwindow* window;
};

auto main(void) -> int {
  TriangleApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}