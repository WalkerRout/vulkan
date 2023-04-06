
#include <iostream>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gtest/gtest.h"

// #include "vulkan.h"
#include "camera.h"

TEST(test_camera, test_camera_creation) {
  auto cam = camera::Camera({1.0f, 2.0f, 3.0f});

  auto test = glm::vec3(1.0f, 2.0f, 3.0f);
  EXPECT_EQ(cam.position, test);
}

TEST(test_camera, test_camera_move) {
  auto cam = camera::Camera({1.0f, 2.0f, 3.0f});
  cam.move({1.0f, 2.0f, 3.0f});

  auto test = glm::vec3(2.0f, 4.0f, 6.0f);
  EXPECT_EQ(cam.position, test);
}