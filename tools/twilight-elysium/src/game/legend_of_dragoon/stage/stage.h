#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace te::lod {

struct StageSpawn {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  std::string character_id;
};

class Stage {
 public:
  Stage() = default;
  void load(const std::string& stage_id);
  const std::vector<StageSpawn>& spawns() const noexcept;

 private:
  std::vector<StageSpawn> spawns_;
};

}  // namespace te::lod
