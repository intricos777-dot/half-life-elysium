#pragma once
#include <cstdint>
#include <vector>
#include "character/character.h"
#include "../world/world.h"

namespace te::lod {

enum class BattleResult : uint32_t { kNone = 0, kVictory, kDefeat };

class BattleArena {
 public:
  BattleArena() = default;
  bool start(const std::string& stage_id);
  BattleResult update();
  void end();

 private:
  std::string stage_id_;
  World world_;
  bool active_ = false;
};

}  // namespace te::lod
