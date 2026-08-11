#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../character/character.h"

namespace te::lod {

struct SkillNode {
  std::string skill_id;
  std::string name;
  uint32_t cost = 1;
  bool unlocked = false;
};

class ProgressionTree {
 public:
  ProgressionTree() = default;
  void initialize_for(CharacterId id);
  bool unlock(CharacterId id, const std::string& skill_id);
  const std::vector<SkillNode>& skills(CharacterId id) const;

 private:
  std::vector<SkillNode> skills_[static_cast<size_t>(CharacterId::kCount)];
};

}  // namespace te::lod
