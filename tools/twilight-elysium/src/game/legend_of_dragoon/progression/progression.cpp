#include "progression.h"

namespace te::lod {

void ProgressionTree::initialize_for(CharacterId id) {
  size_t idx = static_cast<size_t>(id);
  skills_[idx].push_back(SkillNode{"basic-1", "Power Strike", 1, false});
  skills_[idx].push_back(SkillNode{"basic-2", "Guard", 1, false});
}

bool ProgressionTree::unlock(CharacterId id, const std::string& skill_id) {
  size_t idx = static_cast<size_t>(id);
  for (auto& s : skills_[idx]) {
    if (s.skill_id == skill_id && !s.unlocked) {
      s.unlocked = true;
      return true;
    }
  }
  return false;
}

const std::vector<SkillNode>& ProgressionTree::skills(CharacterId id) const {
  return skills_[static_cast<size_t>(id)];
}

}  // namespace te::lod
