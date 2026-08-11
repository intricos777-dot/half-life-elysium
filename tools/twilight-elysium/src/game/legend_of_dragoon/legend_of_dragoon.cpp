#include "legend_of_dragoon.h"

namespace te::lod {

LegendOfDragoon::LegendOfDragoon() = default;

bool LegendOfDragoon::initialize() {
  return true;
}

void LegendOfDragoon::shutdown() {}

World& LegendOfDragoon::world() { return world_; }
CombatInstance& LegendOfDragoon::combat() { return combat_; }

}  // namespace te::lod
