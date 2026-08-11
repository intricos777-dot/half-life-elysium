#pragma once
#include <cstdint>
#include <string>

namespace te {

struct Material {
  std::string name;
  float metallic = 0.0f;
  float roughness = 0.5f;
  float ao = 1.0f;
  uint32_t albedo_texture = 0;
  uint32_t normal_texture = 0;
  uint32_t emissive_texture = 0;
};

}  // namespace te
