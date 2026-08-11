#include "dragoon_data.h"

namespace te::lod {

DragoonDataStore::DragoonDataStore() {
  stages_ = {
    {"intro_forest", "Intro Forest", "forest", 24, 24},
    {"king_fridge", "King Fridge's Castle", "castle", 32, 32},
    {"hellena_prison", "Hellena Prison", "dungeon", 24, 24},
    {"basin_village", "Basin Village", "village", 16, 16},
    {"dragon_ruins", "Dragon Ruins", "ruins", 32, 32},
  };

  progressions_ = {
    {CharacterId::kDart, 110, 25, 18, 12, 16, 8},
    {CharacterId::kLavitz, 120, 20, 16, 15, 14, 6},
    {CharacterId::kShana, 95, 35, 10, 10, 12, 14},
    {CharacterId::kRose, 105, 40, 14, 11, 15, 16},
    {CharacterId::kAlbert, 115, 22, 15, 14, 13, 9},
    {CharacterId::kMeru, 100, 45, 12, 10, 18, 18},
    {CharacterId::kKongol, 130, 18, 20, 10, 10, 5},
    {CharacterId::kMiranda, 108, 50, 11, 9, 11, 20},
  };
}

const std::vector<StageEntry>& DragoonDataStore::stages() const noexcept { return stages_; }
const ProgressionPreset& DragoonDataStore::progression(CharacterId id) const {
  return progressions_[static_cast<size_t>(id)];
}

}  // namespace te::lod
