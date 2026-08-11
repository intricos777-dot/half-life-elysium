#include "input.h"
#include <cstdio>

namespace te {

void InputSystem::pump_events() {}
bool InputSystem::is_key_down(KeyCode key) const {
    uint64_t mask = 1ULL << static_cast<uint32_t>(key);
    return (m_key_state & mask) != 0;
}
void InputSystem::reset_frame() {}

} // namespace te
