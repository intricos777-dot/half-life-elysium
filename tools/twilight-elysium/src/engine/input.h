#pragma once
#include <cstdint>

namespace te {

enum class KeyCode : uint32_t {
    W, A, S, D,
    Space, Shift,
    MouseLeft, MouseRight,
    Escape,
};

enum class InputAction : uint32_t {
    Press, Release, Repeat,
};

struct InputEvent {
    KeyCode key;
    InputAction action;
    uint32_t modifiers;
};

class InputSystem {
public:
    void pump_events();
    bool is_key_down(KeyCode key) const;
    void reset_frame();
private:
    uint64_t m_key_state{0};
};

} // namespace te
