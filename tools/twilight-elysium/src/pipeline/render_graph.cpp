#include "render_graph.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace te {

bool FrameGraph::compile(const RenderGraphDesc& desc) {
    std::printf("[Pipeline] FrameGraph compiled with %zu attachments\n", desc.attachments.size());
    for (size_t i = 0; i < desc.attachments.size(); i++) {
        const auto& a = desc.attachments[i];
        std::printf("[Pipeline]   Attachment %zu: %ux%u format=%u\n", i, a.width, a.height, a.format);
    }
    return !desc.attachments.empty();
}

Pass& RenderGraph::addPass(PassType type) {
    passes_.push_back(Pass{});
    passes_.back().type = type;
    return passes_.back();
}

Pass& RenderGraph::addPass(PassType type, const std::vector<AttachmentDesc>& attachments) {
    Pass& p = addPass(type);
    p.attachments = attachments;
    return p;
}

void RenderGraph::addDraw(uint32_t pass_index, const DrawItem& item) {
    if (pass_index < passes_.size()) passes_[pass_index].draws.push_back(item);
}

// ---------------------------------------------------------------------------
// Frustum: extract 6 planes from view*proj (column-major Mat4::data).
// ---------------------------------------------------------------------------
RenderGraph::Frustum RenderGraph::Frustum::fromMatrix(const Mat4& vp) {
    // Gribb-Hartmann extraction; planes normalized.
    Frustum f;
    const float* m = vp.data;
    // column-major: m[row + 4*col]
    auto plane = [&](int row_sign[4], float* out) {
        // out = row_sign * (row of matrix) where row = (m[c] x-component...)
        for (int r = 0; r < 4; ++r) {
            float val = 0.f;
            for (int c = 0; c < 4; ++c) val += row_sign[c] * m[r + 4 * c];
            out[r] = val;
        }
        float len = std::sqrt(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
        if (len > 1e-8f) for (int r = 0; r < 4; ++r) out[r] /= len;
    };
    plane((int[4]){1, 0, 0, 1}, f.planes[0]);   // left
    plane((int[4]){-1, 0, 0, 1}, f.planes[1]);  // right
    plane((int[4]){0, 1, 0, 1}, f.planes[2]);   // bottom
    plane((int[4]){0, -1, 0, 1}, f.planes[3]);  // top
    plane((int[4]){0, 0, 1, 1}, f.planes[4]);   // near
    plane((int[4]){0, 0, -1, 1}, f.planes[5]);  // far
    return f;
}

bool RenderGraph::intersects(const Frustum& f, const Aabb& b) {
    // AABB vs plane: use the positive vertex (p-vertex) — outside if p-vertex
    // is on the negative side of any plane.
    for (int p = 0; p < 6; ++p) {
        const float* pl = f.planes[p];
        // p-vertex for plane (a,b,c):
        float px = pl[0] >= 0 ? b.max.x : b.min.x;
        float py = pl[1] >= 0 ? b.max.y : b.min.y;
        float pz = pl[2] >= 0 ? b.max.z : b.min.z;
        if (pl[0]*px + pl[1]*py + pl[2]*pz + pl[3] < 0.f) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
bool RenderGraph::compile(const RenderGraphDesc& desc) {
    // Validate attachments: every pass attachment must be non-zero size.
    for (const auto& a : desc.attachments) {
        if (a.width == 0 || a.height == 0) {
            std::fprintf(stderr, "[RenderGraph] invalid attachment %ux%u\n", a.width, a.height);
            return false;
        }
        if (a.format > 3) {
            std::fprintf(stderr, "[RenderGraph] unknown attachment format %u\n", a.format);
            return false;
        }
    }
    built_ = false;
    return true;
}

void RenderGraph::build(const Mat4& view_proj) {
    Frustum f = Frustum::fromMatrix(view_proj);

    // Pass order is fixed by design: Background → Opaque → Skinned → UI.
    std::stable_sort(passes_.begin(), passes_.end(),
                     [](const Pass& a, const Pass& b) { return (uint32_t)a.type < (uint32_t)b.type; });

    char order[4] = {};
    for (auto& p : passes_) {
        // Cull draw items that fall outside the frustum.
        p.draws.erase(
            std::remove_if(p.draws.begin(), p.draws.end(),
                           [&](const DrawItem& it) { return it.lod_index == 0 && !intersects(f, it.bounds); }),
            p.draws.end());
        order[(uint32_t)p.type] = 1;
    }
    built_ = true;
}

std::string RenderGraph::report() const {
    char buf[512] = {};
    int n = std::snprintf(buf, sizeof(buf), "[RenderGraph] %zu passes", passes_.size());
    for (const auto& p : passes_)
        n += std::snprintf(buf + n, sizeof(buf) - (size_t)n,
                           " | pass=%u draws=%zu", (uint32_t)p.type, p.draws.size());
    return std::string(buf);
}

} // namespace te