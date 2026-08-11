#include "world.h"

namespace te::lod {

void World::load_stage(const std::string& stage_id) {
  current_ = StageData{stage_id, stage_id, 32, 32, "default"};
}

const StageData& World::current_stage() const noexcept { return current_; }

}  // namespace te::lod
