#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

#include "engine/math_types.h"

namespace te {

// ============================================================================
// RENDER GRAPH — real replacement for frame_graph_stub.
// Passes (Background → Opaque → Skinned → UI), attachment validation,
// draw-list assembly, and frustum culling per node (AABB vs view-proj).
// ============================================================================

struct AttachmentDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t format = 0;   // 0=R8G8B8A8, 1=D24S8, 2=R16F, 3=R32F
};

struct RenderGraphDesc {
    std::vector<AttachmentDesc> attachments;
};

// Legacy single-shot API (kept for compatibility).
class FrameGraph {
public:
    static bool compile(const RenderGraphDesc& desc);
};

// ---------------------------------------------------------------------------

enum class PassType : uint32_t { Background = 0, Opaque = 1, Skinned = 2, UI = 3 };

struct Aabb {
    Vec3 min{0, 0, 0};
    Vec3 max{0, 0, 0};
};

struct DrawItem {
    std::string model;        // asset registry name (AssetManager)
    std::string material;     // texture set / material name
    Mat4 world = Mat4::identity();
    uint32_t lod_index = 0;
    Aabb bounds;
    bool cast_shadow = true;
};

struct Pass {
    PassType type = PassType::Opaque;
    std::vector<AttachmentDesc> attachments;
    std::vector<DrawItem> draws;
    bool clear_color = false;
    Vec3 clear_rgb{0, 0, 0};
    bool clear_depth = true;
};

class RenderGraph {
public:
    RenderGraph() = default;

    // Pass management.
    Pass& addPass(PassType type);
    Pass& addPass(PassType type, const std::vector<AttachmentDesc>& attachments);
    void addDraw(uint32_t pass_index, const DrawItem& item);

    // Build: validates attachments/ordering, runs culling against the
    // combined view-projection (called each frame with the camera).
    bool compile(const RenderGraphDesc& desc);
    void build(const Mat4& view_proj_combined);

    const std::vector<Pass>& passes() const { return passes_; }
    std::string report() const;

    // Clip-space frustum construction from a column-major Mat4 (view*proj).
    struct Frustum {
        float planes[6][4]; // left, right, bottom, top, near, far (ax+by+cz+d)
        static Frustum fromMatrix(const Mat4& vp);
        bool intersects(const Aabb& b) const;
    };

    static bool intersects(const Frustum& f, const Aabb& b);

private:
    std::vector<Pass> passes_;
    bool built_ = false;
};

} // namespace te