#include "engine.h"
#include "../renderer/renderer.h"
#include <cstdio>
#include <cstring>

namespace {

// Headless smoke: attach a ticking system, run 60 frames, assert the loop
// closes and the tick actually fired. This is what CTest runs as te-core.
int bounded_smoke() {
    te::Engine& engine = te::Engine::instance();
    uint64_t ticks = 0;
    uint64_t last_frame = 0;
    engine.attach_tick([&](float, uint64_t frame) { ++ticks; last_frame = frame; });

    te::EngineConfig cfg;
    cfg.window_title = "Twilight Elysium (headless)";
    cfg.window_width = 320;
    cfg.window_height = 180;
    cfg.scale_mode = 0;
    if (!engine.initialize(cfg)) {
        std::fprintf(stderr, "[smoke] engine init failed\n");
        return 1;
    }
    engine.run(60);   // bounded: the loop must close

    const bool ticked = ticks == 60u && last_frame == 60u;
    std::printf("[smoke] ran %llu frames, tick fired %llu times - %s\n",
                (unsigned long long)last_frame, (unsigned long long)ticks,
                ticked ? "PASS" : "FAIL");
    engine.shutdown();
    return ticked ? 0 : 1;
}
} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--bounded") == 0) return bounded_smoke();
    }

    te::Engine engine;
    te::EngineConfig cfg;
    cfg.window_title = "Twilight Elysium";
    cfg.window_width = 1280;
    cfg.window_height = 720;
    cfg.base_width = 640;
    cfg.base_height = 360;
    cfg.scale_mode = 4; // ScaleMode::Optimized

    if (argc > 1 && std::strcmp(argv[1], "--auto") == 0) cfg.scale_mode = 5;
    else if (argc > 1 && std::strcmp(argv[1], "--integer") == 0) { cfg.scale_mode = 0; cfg.base_width = 1920; cfg.base_height = 1080; }
    else if (argc > 1 && std::strcmp(argv[1], "--fractional") == 0) cfg.scale_mode = 1;
    else if (argc > 1 && std::strcmp(argv[1], "--fsr") == 0) cfg.scale_mode = 3;
    else if (argc > 1 && std::strcmp(argv[1], "--optimized") == 0) cfg.scale_mode = 4;

    if (!engine.initialize(cfg)) {
        std::fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }

    std::printf("[main] engine initialized\n");
    engine.run();
    engine.shutdown();
    return 0;
}