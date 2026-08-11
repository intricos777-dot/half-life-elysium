#pragma once
#include <cstdint>

namespace te {

struct CameraState {
  float x = 0.0f;
  float y = 0.0f;
  float z = 5.0f;
  float pitch = 0.0f;
  float yaw = 0.0f;
  float fov = 60.0f;
  float near_z = 0.1f;
  float far_z = 1000.0f;
};

class ICamera {
 public:
  virtual ~ICamera() = default;
  virtual void update(float delta_ms) = 0;
  virtual CameraState state() const = 0;
};

}  // namespace te
