#pragma once
#include <string>
#include <cstdint>
#include <chrono>
#include <functional>
#include "input.h"

namespace te {

struct EngineConfig {
    std::string window_title = "Twilight Elysium";
    uint32_t window_width = 1280;
    uint32_t window_height = 720;
    bool vulkan_preferred = true;
    bool debug_layers = false;
    uint32_t base_width = 640;
    uint32_t base_height = 360;
    uint32_t scale_mode = 0; // ScaleMode::Auto
};

class Engine {
public:
    Engine() = default;
    ~Engine();

    bool initialize(const EngineConfig& config);

    // Closed-loop runner. Each iteration pumps input, then calls the host
    // tick system (if registered). The loop closes when the host calls
    // request_quick_exit(), Escape is pressed, or `max_frames` frames have
    // run (0 = unbounded). Headless hosts use attach_tick + a frame budget
    // so CI can drive the same ECS the windowed build runs.
    void run(uint32_t max_frames = 0);
    void shutdown();

    // Host system subscription: runs every iteration with (dt, frameNo).
    using TickFn = std::function<void(float, uint64_t)>;
    void attach_tick(TickFn fn) { m_tick = std::move(fn); }

    void request_quick_exit() { m_stop_requested = true; }
    bool stopping() const { return m_stop_requested; }
    InputSystem& input() { return m_input; }
    uint64_t frame() const { return m_frame; }

    static Engine& instance();

private:
    void step(float dt);
    bool m_running = false;
    bool m_initialized = false;
    bool m_stop_requested = false;
    uint64_t m_frame = 0;
    std::chrono::steady_clock::time_point m_last_step{};
    TickFn m_tick;
    InputSystem m_input;
};

} // namespace te