#include "combat.h"

namespace te::lod {

void CombatInstance::add_ally(CharacterId id) { allies_.push_back(id); }
void CombatInstance::add_enemy(CharacterId id) { enemies_.push_back(id); }

std::vector<CombatTurn> CombatInstance::resolve_turn() {
  std::vector<CombatTurn> turns;
  for (auto id : allies_) {
    turns.push_back(CombatTurn{id, 0, false});
  }
  for (auto id : enemies_) {
    turns.push_back(CombatTurn{id, 0, false});
  }
  return turns;
}

}  // namespace te::lod
