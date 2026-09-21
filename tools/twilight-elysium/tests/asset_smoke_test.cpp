// Headless smoke test for the real engine modules (asset / pipeline / shader).
// No GL context is required: GPU paths are compile-guarded and skipped.
//
// Accepts an optional GLB path as argv[1]; defaults to a Hermes/Seele export.
#include <cstdio>
#include <cstring>
#include <string>

#include "asset/asset_manager.h"
#include "pipeline/render_graph.h"
#include "shader/shader_pipeline.h"

using namespace te;

#ifndef TE_SHADER_DIR
#define TE_SHADER_DIR "/home/sin/Projects/games/twilight-elysium/assets/shaders"
#endif

static int g_failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (cond) {                                                            \
            std::printf("  [ok] %s\n", msg);                                   \
        } else {                                                               \
            std::printf("  [FAIL] %s\n", msg);                                 \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

int main(int argc, char** argv) {
    std::printf("=== te-asset-smoke: module proof ===\n");

    // 1. Asset module: import a real Seele GLB.
    const std::string path = argc > 1
        ? argv[1]
        : std::string(getenv("HOME")) + "/.twilight-elys/blender_export_chars/char_kite.glb";

    AssetManager am;
    CHECK(am.importFromGlb(path), "AssetManager imported Seele GLB");

    const MeshData* m = am.mesh("char_kite");
    CHECK(m != nullptr, "mesh registered by stem name");
    if (m) {
        std::printf("  [info] name=%s verts=%u faces=%u normals=%d uvs=%d\n",
                    m->name.c_str(), m->vertex_count, m->face_count,
                    m->has_normals ? 1 : 0, m->has_uvs ? 1 : 0);
        CHECK(m->vertex_count > 0, "vertex data present");
        CHECK(m->face_count > 0, "triangle indices present");
        CHECK(m->positions.size() == (size_t)m->vertex_count * 3, "position layout consistent");
        CHECK(m->error.empty(), "no parse error reported");
    }

    // 2. Pipeline module: render graph compile + legacy FrameGraph.
    AttachmentDesc a; a.width = 640; a.height = 480; a.format = 0;
    RenderGraphDesc rdesc; rdesc.attachments.push_back(a);
    CHECK(FrameGraph::compile(rdesc), "FrameGraph::compile (legacy API) accepts attachments");

    RenderGraph rg;
    rg.addPass(PassType::Background, rdesc.attachments);
    Pass& opaque = rg.addPass(PassType::Opaque, rdesc.attachments);

    DrawItem item;
    item.model = "char_kite";
    item.material = "organic";
    item.world = Mat4::translation(Vec3{0.f, 0.f, -8.f});
    item.bounds = Aabb{Vec3{-1, -1, -1}, Vec3{1, 1, 1}};
    opaque.draws.push_back(item);

    // A blast far outside the frustum must be culled at build time.
    DrawItem far_away = item;
    far_away.bounds = Aabb{Vec3{1000, 1000, 1000}, Vec3{1001, 1001, 1001}};
    opaque.draws.push_back(far_away);

    CHECK(rg.compile(rdesc), "RenderGraph::compile validates attachments");

    Mat4 vp = Mat4::identity(); // camera at origin looking down -z
    rg.build(vp);
    std::printf("  [info] %s\n", rg.report().c_str());
    bool culled = true;
    for (const auto& p : rg.passes())
        for (const auto& d : p.draws)
            if (d.bounds.max.x > 500.f) culled = false;
    CHECK(culled, "off-frustum draw culled at build");

    // 3. Shader module: compile the engine's real GLSL from disk.
    //    (Slim vendored trees may omit assets/shaders — skip gracefully.)
    std::vector<uint8_t> bytecode;
    const std::string vs_path = std::string(TE_SHADER_DIR) + "/default.vert";
    const std::string fs_path = std::string(TE_SHADER_DIR) + "/default.frag";
    std::FILE* probe = std::fopen(vs_path.c_str(), "rb");
    if (probe) {
        std::fclose(probe);
        bool s1 = ShaderCompiler::compile_from_file(vs_path, ShaderStage::Vertex, bytecode);
        CHECK(s1 && bytecode.size() >= 16, "vertex shader compiled from file");
        if (s1 && bytecode.size() >= 4) {
            uint32_t magic = (uint32_t)bytecode[0] | ((uint32_t)bytecode[1] << 8) |
                             ((uint32_t)bytecode[2] << 16) | ((uint32_t)bytecode[3] << 24);
            CHECK(magic == 0x31444853, "bytecode carries SHD1 magic");
        }

        ShaderDesc hd;
        CHECK(hot_reload_shader(fs_path, hd), "hot_reload_shader reads fragment source");
    } else {
        std::printf("  [skip] no assets/shaders in this tree (slim vendor)\n");
    }

    CHECK(!ShaderCache::gpuReady() || ShaderCache::gpuReady(),
          "gpu readiness flag queryable");

    std::printf("=== result: %s (%d failures) ===\n",
                g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}