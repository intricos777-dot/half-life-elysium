#pragma once
#include <string>
#include <cstdint>

namespace te::lod {

enum class CharacterId : uint32_t {
  kDart = 0,
  kLavitz,
  kShana,
  kRose,
  kAlbert,
  kMeru,
  kKongol,
  kMiranda,
  kCount
};

struct CharacterStats {
  uint32_t level = 1;
  uint32_t hp = 100;
  uint32_t max_hp = 100;
  uint32_t mp = 20;
  uint32_t max_mp = 20;
  uint32_t attack = 10;
  uint32_t defense = 8;
  uint32_t speed = 12;
  uint32_t magic = 6;
};

class Character {
 public:
  Character(CharacterId id, std::string name);
  CharacterId id() const noexcept;
  const std::string& name() const noexcept;
  const CharacterStats& stats() const noexcept;
  CharacterStats& mutable_stats();

 private:
  CharacterId id_;
  std::string name_;
  CharacterStats stats_;
};

}  // namespace te::lod
