#include "engine.h"
#include "resolution_scaler.h"
#include <iostream>
#include <thread>

namespace te {

namespace {
constexpr float kFrameBudget = 1.0f / 60.0f;   // 60 Hz tick budget
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const EngineConfig& config) {
    if (m_initialized) return true;
    std::cout << "[Engine] Initializing: " << config.window_title
              << " " << config.window_width << "x" << config.window_height << "\n";
    auto& scaler = ResolutionScaler::instance();
    scaler.set_base_resolution(config.base_width, config.base_height);
    scaler.set_mode((ScaleMode)config.scale_mode);
    scaler.set_window_size(config.window_width, config.window_height);
    m_running = true;
    m_initialized = true;
    m_stop_requested = false;
    m_frame = 0;
    m_last_step = std::chrono::steady_clock::now();
    return true;
}

void Engine::step(float dt) {
    // 1) input pump - headless builds see no events; Escape closes the loop
    m_input.pump_events();
    if (m_input.is_key_down(KeyCode::Escape)) m_stop_requested = true;

    // 2) the registered host system (game logic, ECS update, render pass)
    ++m_frame;
    if (m_tick) m_tick(dt, m_frame);
}

void Engine::run(uint32_t max_frames) {
    if (!m_initialized) return;
    m_stop_requested = false;

    // closed loop: tick at 60 Hz; host tick + frame budget decide the end
    while (m_running && !m_stop_requested) {
        const auto before = std::chrono::steady_clock::now();

        step(kFrameBudget);

        if (max_frames > 0 && m_frame >= max_frames) break;

        // pace to the 60 Hz budget so headless runs are honest but fast
        const auto after = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                                 after - before)
                                 .count();
        const auto target_us = (int64_t)(kFrameBudget * 1e6);
        if (elapsed < target_us)
            std::this_thread::sleep_for(
                std::chrono::microseconds(target_us - elapsed));
    }
}

void Engine::shutdown() {
    if (!m_initialized) return;
    std::cout << "[Engine] Shutting down\n";
    m_running = false;
    m_stop_requested = true;
    m_initialized = false;
}

Engine& Engine::instance() {
    static Engine instance;
    return instance;
}

} // namespace te