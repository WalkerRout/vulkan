#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace camera {

struct Camera {
  Camera() = default;
  Camera(glm::vec3 p);

// ---- Start of Utility Functions ----
public:
  auto move(glm::vec3 offset) -> void;
  auto set_position(glm::vec3 new_p) -> void;
private:
  // N/A
// ---- End of Utility Functions ----

// ---- Start of Class Members ----
public:
  glm::vec3 position{};
private:
  // N/A
// ---- End of Class Members ----
};

} // end of namespace camera

#endif // CAMERA_H