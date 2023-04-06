#include <iostream>

#include "vulkan.h"

using namespace vulkan;

auto main(void) -> int {
  try {
    VulkanApplication app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}