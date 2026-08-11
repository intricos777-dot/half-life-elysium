#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace te::lod {

struct StageData {
  std::string stage_id;
  std::string name;
  uint32_t width_tiles = 0;
  uint32_t height_tiles = 0;
  std::string environment;
};

class World {
 public:
  World() = default;
  void load_stage(const std::string& stage_id);
  const StageData& current_stage() const noexcept;

 private:
  std::vector<StageData> stages_;
  StageData current_;
};

}  // namespace te::lod
