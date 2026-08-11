#include "battle.h"

namespace te::lod {

bool BattleArena::start(const std::string& stage_id) {
  stage_id_ = stage_id;
  world_.load_stage(stage_id);
  active_ = true;
  return true;
}

BattleResult BattleArena::update() { return active_ ? BattleResult::kNone : BattleResult::kVictory; }
void BattleArena::end() { active_ = false; }

}  // namespace te::lod
