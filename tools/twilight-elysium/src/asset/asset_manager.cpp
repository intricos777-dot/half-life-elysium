#include "asset_manager.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <array>
#include <string>

// GL includes are guarded: the importer runs headless; only the uploader
// functions require a live OpenGL context.
#if defined(TE_HAS_GL)
#include <GL/glew.h>
#endif

#include <nlohmann/json.hpp>

namespace te {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// AssetLoader (raw file IO)
// ---------------------------------------------------------------------------
bool AssetLoader::load(const std::string& path, std::vector<uint8_t>& out_data) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    if (sz < 0) { std::fclose(f); return false; }
    std::rewind(f);
    out_data.resize(static_cast<size_t>(sz));
    size_t rd = std::fread(out_data.data(), 1, out_data.size(), f);
    std::fclose(f);
    return rd == out_data.size();
}

// ---------------------------------------------------------------------------
// GLB parsing
// ---------------------------------------------------------------------------
namespace {

constexpr uint32_t kGlbMagic = 0x46546C67;        // "glTF"
constexpr uint32_t kChunkJson = 0x4E4F534A;       // "JSON"
constexpr uint32_t kChunkBin  = 0x004E4942;       // "BIN\x00"
constexpr uint32_t kF32       = 5126;
constexpr uint32_t kU16       = 5123;
constexpr uint32_t kU32       = 5125;
constexpr uint32_t kTriangles = 4;

uint32_t readU32(const std::vector<uint8_t>& b, size_t off) {
    return static_cast<uint32_t>(b[off]) |
           (static_cast<uint32_t>(b[off + 1]) << 8) |
           (static_cast<uint32_t>(b[off + 2]) << 16) |
           (static_cast<uint32_t>(b[off + 3]) << 24);
}

float readF32(const std::vector<uint8_t>& b, size_t off) {
    uint32_t u = readU32(b, off);
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

int typeCount(const std::string& t) {
    if (t == "SCALAR") return 1;
    if (t == "VEC2") return 2;
    if (t == "VEC3") return 3;
    if (t == "VEC4") return 4;
    if (t == "MAT4") return 16;
    return 0;
}

size_t componentSize(uint32_t ct) {
    switch (ct) {
        case 5120: return 1; case 5121: return 1;
        case 5122: return 2; case 5123: return 2;
        case 5125: return 4; case 5126: return 4;
        default: return 0;
    }
}

} // namespace

bool GlbImporter::parse(const std::vector<uint8_t>& bytes, MeshData& out) {
    out = MeshData{};
    if (bytes.size() < 20 || readU32(bytes, 0) != kGlbMagic) {
        out.error = "not a GLB (bad magic)";
        return false;
    }
    const uint32_t version = readU32(bytes, 4);
    if (version != 2) { out.error = "unsupported GLB version " + std::to_string(version); return false; }

    // Locate JSON and BIN chunks.
    struct Slice { size_t begin, end; };
    Slice jsonS{0, 0}, binS{0, 0};
    size_t off = 12;
    bool foundJson = false, foundBin = false;
    while (off + 8 <= bytes.size()) {
        uint32_t len = readU32(bytes, off);
        uint32_t type = readU32(bytes, off + 4);
        if (off + 8 + len > bytes.size()) { out.error = "chunk length overflow"; return false; }
        if (type == kChunkJson) { jsonS = {off + 8, off + 8 + len}; foundJson = true; }
        else if (type == kChunkBin) { binS = {off + 8, off + 8 + len}; foundBin = true; }
        off += 8 + len;
    }
    if (!foundJson) { out.error = "missing JSON chunk"; return false; }
    (void)foundBin;

    json root;
    try {
        root = json::parse(bytes.begin() + jsonS.begin, bytes.begin() + jsonS.end);
    } catch (const std::exception& e) {
        out.error = std::string("JSON parse: ") + e.what();
        return false;
    }

    // bufferViews
    const auto& views = root.value("bufferViews", json::array());
    std::vector<std::array<size_t, 4>> view; // {begin, end, stride, target}
    for (const auto& v : views) {
        size_t begin = binS.begin;
        if (v.contains("byteOffset")) begin += v["byteOffset"].get<size_t>();
        size_t len = v.value("byteLength", size_t(0));
        size_t stride = v.contains("byteStride") ? v["byteStride"].get<size_t>() : 0;
        size_t target = v.value("target", size_t(0));
        view.push_back({begin, begin + len, stride, target});
    }

    const auto& accessors = root.value("accessors", json::array());
    struct Acc { size_t viewIndex; size_t count; uint32_t compType; uint32_t comps; size_t byteOffset; };
    std::vector<Acc> acc;
    for (const auto& a : accessors) {
        Acc ad{};
        ad.viewIndex = a.contains("bufferView") ? a["bufferView"].get<size_t>() : SIZE_MAX;
        ad.count = a.value("count", size_t(0));
        ad.compType = a.value("componentType", uint32_t(0));
        ad.comps = static_cast<uint32_t>(typeCount(a.value("type", std::string("SCALAR"))));
        ad.byteOffset = a.contains("byteOffset") ? a["byteOffset"].get<size_t>() : 0;
        if (a.contains("count")) ad.count = a["count"].get<size_t>();
        acc.push_back(ad);
    }

    auto fetch = [&](size_t accIndex, uint32_t comps, std::vector<float>& dst) -> bool {
        if (accIndex == SIZE_MAX || accIndex >= acc.size()) return false;
        const Acc& a = acc[accIndex];
        if (a.comps < comps) return false;
        if (a.viewIndex == SIZE_MAX || a.viewIndex >= view.size()) return false;
        const auto& v = view[a.viewIndex];
        const size_t stride = v[2] ? v[2] : a.comps * componentSize(a.compType);
        size_t p = v[0] + a.byteOffset;
        for (size_t i = 0; i < a.count; ++i) {
            for (uint32_t c = 0; c < comps; ++c) {
                if (a.compType == kF32) dst.push_back(readF32(bytes, p + c * 4));
                else { out.error = "GLB: non-float attribute component"; return false; }
            }
            p += stride;
        }
        return true;
    };

    // First mesh's first primitive drives the asset; merge additional
    // primitives with index offsets.
    uint32_t indexBase = 0;
    const auto& meshes = root.value("meshes", json::array());
    for (const auto& m : meshes) {
        const auto& prims = m.value("primitives", json::array());
        for (const auto& prim : prims) {
            uint32_t mode = prim.value("mode", uint32_t(kTriangles));
            if (mode != kTriangles) continue;
            const auto& attrs = prim.value("attributes", json::object());
            if (!attrs.contains("POSITION")) { out.error = "primitive missing POSITION"; return false; }

            std::vector<float> pos, nrm, uv;
            if (!fetch(attrs["POSITION"].get<size_t>(), 3, pos)) { out.error = "bad POSITION accessor"; return false; }
            if (out.name.empty()) out.name = m.value("name", std::string("mesh"));
            const bool hasN = attrs.contains("NORMAL") && fetch(attrs["NORMAL"].get<size_t>(), 3, nrm);
            const bool hasU = attrs.contains("TEXCOORD_0") && fetch(attrs["TEXCOORD_0"].get<size_t>(), 2, uv);

            std::vector<uint32_t> idx;
            if (prim.contains("indices")) {
                size_t ii = prim["indices"].get<size_t>();
                if (ii >= acc.size()) { out.error = "bad indices accessor"; return false; }
                const Acc& a = acc[ii];
                if (a.viewIndex == SIZE_MAX || a.viewIndex >= view.size()) { out.error = "indices view missing"; return false; }
                const auto& v = view[a.viewIndex];
                const size_t stride = v[2] ? v[2] : componentSize(a.compType);
                size_t p = v[0] + a.byteOffset;
                for (size_t i = 0; i < a.count; ++i) {
                    if (a.compType == kU16) idx.push_back(static_cast<uint32_t>(bytes[p] | (bytes[p + 1] << 8)));
                    else if (a.compType == kU32) idx.push_back(readU32(bytes, p));
                    else { out.error = "GLB: unsupported index component type"; return false; }
                    p += stride;
                }
            } else {
                for (size_t i = 0; i < pos.size() / 3; ++i) idx.push_back(static_cast<uint32_t>(i));
            }

            const size_t vert = pos.size() / 3;
            if (out.positions.empty()) {
                out.positions = std::move(pos);
                out.normals = std::move(nrm);
                out.uvs = std::move(uv);
                out.has_normals = hasN && !out.normals.empty();
                out.has_uvs = hasU && !out.uvs.empty();
            } else {
                // append interleaved-per-vertex: must match vertex layout of first
                out.positions.insert(out.positions.end(), pos.begin(), pos.end());
                if (hasN) out.normals.insert(out.normals.end(), nrm.begin(), nrm.end());
                if (hasU) out.uvs.insert(out.uvs.end(), uv.begin(), uv.end());
            }
            for (uint32_t i : idx) out.indices.push_back(i + indexBase);
            // For multi-primitive assets: recompute base for the next primitive.
            (void)vert;
            indexBase = static_cast<uint32_t>(out.positions.size() / 3);
            (void)indexBase;
        }
    }

    if (out.positions.empty()) { out.error = "no triangle primitives in GLB"; return false; }
    out.vertex_count = static_cast<uint32_t>(out.positions.size() / 3);
    out.face_count = static_cast<uint32_t>(out.indices.size() / 3);
    out.error.clear();
    return true;
}

bool GlbImporter::importFile(const std::string& path, MeshData& out) {
    std::vector<uint8_t> bytes;
    if (!AssetLoader::load(path, bytes)) { out.error = "cannot read file: " + path; return false; }
    return parse(bytes, out);
}

// ---------------------------------------------------------------------------
// GPU upload
// ---------------------------------------------------------------------------
bool AssetManager::gpuAvailable() {
#if defined(TE_HAS_GL)
    return true;
#else
    return false;
#endif
}

bool AssetManager::importFromGlb(const std::string& path) {
    size_t slash = path.find_last_of('/');
    std::string stem = path;
    if (slash != std::string::npos) stem = path.substr(slash + 1);
    size_t dot = stem.find_last_of('.');
    if (dot != std::string::npos) stem = stem.substr(0, dot);

    MeshData md;
    if (!GlbImporter::importFile(path, md)) return false;
    md.name = stem;
    meshes_[stem] = std::move(md);

    // Stamp a house-loader BlenderModel from the imported mesh.
    seele::BlenderModel bm;
    bm.name = stem;
    bm.path = path;
    bm.format = "glTF";
    const MeshData& stored = meshes_[stem];
    bm.vertex_count = stored.vertex_count;
    bm.face_count = stored.face_count;
    blender_[stem] = bm;
    return true;
}

bool AssetManager::uploadToGpu(const std::string& name) {
    auto it = meshes_.find(name);
    if (it == meshes_.end()) return false;
#if defined(TE_HAS_GL)
    const MeshData& m = it->second;
    if (gpu_.count(name) && gpu_[name].uploaded) return true;

    // Interleave POSITION (3), NORMAL (3), TEXCOORD (2) = 8 floats.
    std::vector<float> interleaved;
    interleaved.reserve(m.vertex_count * 8);
    for (size_t i = 0; i < m.vertex_count; ++i) {
        interleaved.push_back(m.positions[i * 3 + 0]);
        interleaved.push_back(m.positions[i * 3 + 1]);
        interleaved.push_back(m.positions[i * 3 + 2]);
        if (m.has_normals) {
            interleaved.push_back(m.normals[i * 3 + 0]);
            interleaved.push_back(m.normals[i * 3 + 1]);
            interleaved.push_back(m.normals[i * 3 + 2]);
        } else { interleaved.insert(interleaved.end(), {0.f, 0.f, 1.f}); }
        if (m.has_uvs) {
            interleaved.push_back(m.uvs[i * 2 + 0]);
            interleaved.push_back(m.uvs[i * 2 + 1]);
        } else { interleaved.insert(interleaved.end(), {0.f, 0.f}); }
    }

    GpuMesh gm;
    glGenVertexArrays(1, &gm.vao);
    glBindVertexArray(gm.vao);
    glGenBuffers(1, &gm.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &gm.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.indices.size() * sizeof(uint32_t), m.indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    gm.index_count = static_cast<uint32_t>(m.indices.size());
    gm.uploaded = true;
    gpu_[name] = gm;
    return true;
#else
    (void)name;
    return false;
#endif
}

bool AssetManager::uploadTextureFromRgba(const std::string& name, uint32_t w, uint32_t h,
                                         const std::vector<uint8_t>& rgba) {
#if defined(TE_HAS_GL)
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    textures_[name] = id;
    return true;
#else
    (void)name; (void)w; (void)h; (void)rgba;
    return false;
#endif
}

bool AssetManager::registerTextureSet(const std::string& name) {
    // Reserve slot with material constants; PNG decode requires a codec —
    // the preferred entry is uploadTextureFromRgba (pairs with seele-style
    // procedural material buffers).
    textures_.emplace(name, 0);
    return true;
}

const MeshData* AssetManager::mesh(const std::string& name) const {
    auto it = meshes_.find(name);
    return it == meshes_.end() ? nullptr : &it->second;
}

const GpuMesh* AssetManager::gpuMesh(const std::string& name) const {
    auto it = gpu_.find(name);
    return it == gpu_.end() ? nullptr : &it->second;
}

const seele::BlenderModel* AssetManager::blenderModel(const std::string& name) const {
    auto it = blender_.find(name);
    return it == blender_.end() ? nullptr : &it->second;
}

std::vector<std::string> AssetManager::names() const {
    std::vector<std::string> out;
    out.reserve(meshes_.size());
    for (const auto& kv : meshes_) out.push_back(kv.first);
    return out;
}

} // namespace te