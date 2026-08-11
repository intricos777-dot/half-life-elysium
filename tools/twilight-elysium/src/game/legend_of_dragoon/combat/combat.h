#pragma once
#include <cstdint>
#include <vector>
#include "../character/character.h"

namespace te::lod {

struct CombatTurn {
  CharacterId actor;
  uint32_t damage = 0;
  bool used_magic = false;
};

class CombatInstance {
 public:
  CombatInstance() = default;
  void add_ally(CharacterId id);
  void add_enemy(CharacterId id);
  std::vector<CombatTurn> resolve_turn();

 private:
  std::vector<CharacterId> allies_;
  std::vector<CharacterId> enemies_;
};

}  // namespace te::lod
