#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <functional>

#include "ai/blender_asset_loader.h" // te::seele::BlenderModel (house loader type)

namespace te {

// ============================================================================
// ASSET MODULE — real replacement for asset_stub.
// Imports glTF/GLB models into BlenderModel types (ai/blender_asset_loader.h),
// manages a named asset registry, and uploads meshes/textures to the GPU
// when an OpenGL context is present.
// ============================================================================

struct AssetMetadata {
    std::string path;
    uint64_t size = 0;
    uint32_t type = 0;   // 0=unknown, 1=glb, 2=gltf, 3=texture, 4=shader
};

// Legacy raw-loader API (kept: harmless real file IO).
class AssetLoader {
public:
    static bool load(const std::string& path, std::vector<uint8_t>& out_data);
};

// ---------------------------------------------------------------------------
// GLB importer (CPU side). Produces plain mesh data from a binary glTF 2.0 file.
// ---------------------------------------------------------------------------
struct MeshData {
    std::string name;
    std::vector<float> positions;   // xyz triples
    std::vector<float> normals;     // xyz triples
    std::vector<float> uvs;         // uv pairs
    std::vector<uint32_t> indices;  // triangles
    uint32_t vertex_count = 0;
    uint32_t face_count = 0;
    bool has_normals = false;
    bool has_uvs = false;
    std::string error;              // last parse error message
};

class GlbImporter {
public:
    // Parses `file_bytes` (full GLB). Returns false + `.error` on failure.
    static bool parse(const std::vector<uint8_t>& file_bytes, MeshData& out);

    // Convenience: reads the file then parses.
    static bool importFile(const std::string& path, MeshData& out);
};

// ---------------------------------------------------------------------------
// GPU uploader (guarded: requires a live GL context).
// ---------------------------------------------------------------------------
struct GpuMesh {
    uint32_t vao = 0, vbo = 0, ebo = 0;
    uint32_t index_count = 0;
    bool uploaded = false;
};

// ---------------------------------------------------------------------------
// Asset registry. Names are derived from the file stem.
// ---------------------------------------------------------------------------
class AssetManager {
public:
    // Import a .glb from disk into the registry (CPU only). Fills the
    // BlenderModel entry (vertex/face counts, format "glTF").
    bool importFromGlb(const std::string& path);

    // Upload a registered model to the GPU (needs GL context; no-op otherwise).
    bool uploadToGpu(const std::string& name);

    // Upload raw RGBA pixels as a GL texture (needs GL context).
    bool uploadTextureFromRgba(const std::string& name, uint32_t w, uint32_t h,
                               const std::vector<uint8_t>& rgba);

    // Upload a PBR texture set from disk (PNG/JPEG unsupported without a
    // codec — callers pass RGBA via uploadTextureFromRgba; this reserves the
    // slot with the material constants).
    bool registerTextureSet(const std::string& name);

    const MeshData* mesh(const std::string& name) const;
    const GpuMesh* gpuMesh(const std::string& name) const;
    // House-loader view: BlenderModel stamped from the imported GLB.
    const seele::BlenderModel* blenderModel(const std::string& name) const;
    std::vector<std::string> names() const;

    size_t count() const { return meshes_.size(); }

    static bool gpuAvailable();

private:
    std::unordered_map<std::string, MeshData> meshes_;
    std::unordered_map<std::string, GpuMesh> gpu_;
    std::unordered_map<std::string, seele::BlenderModel> blender_;
    std::unordered_map<std::string, uint32_t> textures_; // name -> GL id
};

} // namespace te