#pragma once
#include <memory>
#include "character/character.h"
#include "world/world.h"
#include "combat/combat.h"

namespace te::lod {

class LegendOfDragoon {
 public:
  LegendOfDragoon();
  bool initialize();
  void shutdown();

  World& world();
  CombatInstance& combat();

 private:
  World world_;
  CombatInstance combat_;
};

}  // namespace te::lod
