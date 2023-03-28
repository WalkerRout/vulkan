
#include <iostream>
#include <stdexcept>

#include "gtest/gtest.h"

#include "triangle.h"

TEST(test_triangle, test_app_creation) {
  try {
    triangle::TriangleApplication app;
    app.run();
    EXPECT_TRUE(true);
  } catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
}