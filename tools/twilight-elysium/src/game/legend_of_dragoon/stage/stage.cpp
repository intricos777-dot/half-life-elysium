#include "stage.h"

namespace te::lod {

void Stage::load(const std::string& stage_id) {
  spawns_.push_back(StageSpawn{0.0f, 0.0f, 0.0f, "player"});
}

const std::vector<StageSpawn>& Stage::spawns() const noexcept { return spawns_; }

}  // namespace te::lod
