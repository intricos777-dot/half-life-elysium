#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "character/character.h"

namespace te::lod {

struct StageEntry {
  std::string stage_id;
  std::string name;
  std::string environment;
  uint32_t width_tiles = 32;
  uint32_t height_tiles = 32;
};

struct ProgressionPreset {
  CharacterId id;
  uint32_t base_hp;
  uint32_t base_mp;
  uint32_t base_attack;
  uint32_t base_defense;
  uint32_t base_speed;
  uint32_t base_magic;
};

class DragoonDataStore {
 public:
  DragoonDataStore();
  const std::vector<StageEntry>& stages() const noexcept;
  const ProgressionPreset& progression(CharacterId id) const;

 private:
  std::vector<StageEntry> stages_;
  std::vector<ProgressionPreset> progressions_;
};

}  // namespace te::lod
