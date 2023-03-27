#include <iostream>

#include "triangle.h"

using namespace triangle;

auto main(void) -> int {
  try {
    TriangleApplication app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}