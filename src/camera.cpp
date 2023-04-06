
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

namespace camera {

Camera::Camera(glm::vec3 p): position(p) {}

auto Camera::move(glm::vec3 offset) -> void {
  position += offset;
}

auto Camera::set_position(glm::vec3 new_p) -> void {
  position = new_p;
}

}