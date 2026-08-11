#include "menu.h"

namespace te::lod {

void MenuSystem::open(MenuScreen screen) { current_ = screen; }
MenuScreen MenuSystem::current() const noexcept { return current_; }
void MenuSystem::update() {}

}  // namespace te::lod
