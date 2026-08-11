#include "resolution_scaler.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace te {

ResolutionScaler& ResolutionScaler::instance() {
    static ResolutionScaler scaler;
    return scaler;
}

void ResolutionScaler::set_mode(ScaleMode mode) {
    m_mode = mode;
    recalculate();
}

void ResolutionScaler::set_base_resolution(uint32_t width, uint32_t height) {
    m_base.width = width;
    m_base.height = height;
    recalculate();
}

void ResolutionScaler::set_window_size(uint32_t width, uint32_t height) {
    (void)width; (void)height;
    recalculate();
}

void ResolutionScaler::recalculate() {
    uint32_t prev_scale = m_current_scale;

    if (m_mode == ScaleMode::Auto) {
        m_current_scale = 1;
    } else if (m_mode == ScaleMode::Integer) {
        m_current_scale = std::max(1u, m_base.framebuffer_scale);
    } else if (m_mode == ScaleMode::Fractional) {
        m_current_scale = 1;
    } else if (m_mode == ScaleMode::Optimized) {
        // Balanced quality/performance: prefer 1.5x effective load via base resolution bump
        m_current_scale = 1;
        if (m_base.width < 1920 && m_base.height < 1080) {
            m_base.width = 1920;
            m_base.height = 1080;
            std::printf("[Scaler] Optimized: base resolution bumped to 1920x1080\n");
        }
    } else {
        m_current_scale = 1;
    }

    m_scaled.width = m_base.width * m_current_scale;
    m_scaled.height = m_base.height * m_current_scale;
    m_scaled.framebuffer_scale = m_current_scale;

    if (m_mode != ScaleMode::Optimized && m_current_scale > 2) {
        std::printf("[Scaler] Warning: scale factor %u may impact performance\n", m_current_scale);
    }

    std::printf("[Scaler] mode=%u base=%ux%u scale=%u scaled=%ux%u\n",
        (uint32_t)m_mode, m_base.width, m_base.height, m_current_scale, m_scaled.width, m_scaled.height);
}

} // namespace te
