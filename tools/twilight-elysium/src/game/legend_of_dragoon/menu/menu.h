#pragma once
#include <cstdint>
#include <vector>
#include "../character/character.h"

namespace te::lod {

enum class MenuScreen : uint32_t {
  kNone = 0,
  kMain,
  kParty,
  kItems,
  kSettings
};

class MenuSystem {
 public:
  MenuSystem() = default;
  void open(MenuScreen screen);
  MenuScreen current() const noexcept;
  void update();

 private:
  MenuScreen current_ = MenuScreen::kNone;
};

}  // namespace te::lod
