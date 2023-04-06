
#include <iostream>
#include <stdexcept>

#include "gtest/gtest.h"

#include "vulkan.h"

TEST(test_vulkan, test_app_creation) {
  try {
    vulkan::VulkanApplication app;
    app.run();
    EXPECT_TRUE(true);
  } catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
}