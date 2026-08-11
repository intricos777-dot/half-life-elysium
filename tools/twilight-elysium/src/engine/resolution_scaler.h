#pragma once
#include <cstdint>

namespace te {

enum class ScaleMode : uint32_t {
    Integer,        // 1x, 2x, 3x...
    Fractional,     // e.g. 1.5x
    FitToWindow,    // scale to fill window maintaining aspect
    FSR,            // FSR-style upscale then sharpen
    Optimized,      // auto-balanced quality/performance with hardware-aware heuristics
    Auto,           // pick best for current GPU/CPU load
};

struct Resolution {
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t framebuffer_scale = 1; // 1, 2, 3...
};

class ResolutionScaler {
public:
    static ResolutionScaler& instance();
    void set_mode(ScaleMode mode);
    void set_base_resolution(uint32_t width, uint32_t height);
    void set_window_size(uint32_t width, uint32_t height);
    Resolution get_base_resolution() const { return m_base; }
    Resolution get_scaled_resolution() const { return m_scaled; }
    uint32_t get_scale_factor() const { return m_current_scale; }
private:
    ResolutionScaler() = default;
    void recalculate();
    ScaleMode m_mode = ScaleMode::Auto;
    Resolution m_base{1280, 720, 1};
    Resolution m_scaled{1280, 720, 1};
    uint32_t m_current_scale = 1;
};

} // namespace te
