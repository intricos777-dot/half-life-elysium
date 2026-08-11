#include "character.h"

namespace te::lod {

Character::Character(CharacterId id, std::string name)
    : id_(id), name_(std::move(name)) {}

CharacterId Character::id() const noexcept { return id_; }
const std::string& Character::name() const noexcept { return name_; }
const CharacterStats& Character::stats() const noexcept { return stats_; }
CharacterStats& Character::mutable_stats() { return stats_; }

}  // namespace te::lod
