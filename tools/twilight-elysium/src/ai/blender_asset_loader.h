#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace te::seele {

// ============================================================================
// BLENDER-SEELE ASSET TYPES
// ============================================================================

struct BlenderModel {
    std::string name;
    std::string path;        // Path to .glb file
    std::string format;      // "glTF", "OBJ", etc.
    uint32_t vertex_count = 0;
    uint32_t face_count = 0;
    
    // GPU-resident data after upload
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t index_count = 0;
    bool uploaded = false;
};

struct PBRTextureSet {
    std::string name;
    std::string albedo_path;
    std::string normal_path;
    std::string roughness_path;
    std::string metallic_path;
    std::string ao_path;
    
    // GPU handles
    uint32_t albedo_tex = 0;
    uint32_t normal_tex = 0;
    uint32_t roughness_tex = 0;
    uint32_t metallic_tex = 0;
    uint32_t ao_tex = 0;
};

struct BlenderManifest {
    std::vector<BlenderModel> models;
    uint32_t count = 0;
    
    // Lookup by name
    const BlenderModel* find(const std::string& name) const {
        for (const auto& m : models) {
            if (m.name == name) return &m;
        }
        return nullptr;
    }
};

// ============================================================================
// BLENDER ASSET LOADER
// ============================================================================

class BlenderAssetLoader {
public:
    BlenderAssetLoader() = default;
    ~BlenderAssetLoader() = default;
    
    // Load manifest from Blender export directory
    bool load_manifest(const std::string& manifest_path);
    bool load_manifest_from_dir(const std::string& export_dir);
    
    // Get loaded manifest
    const BlenderManifest& manifest() const { return m_manifest; }
    
    // Load individual model into GPU memory
    bool upload_model(const std::string& name);
    bool upload_model(const BlenderModel& model);
    
    // Load all models
    bool upload_all_models();
    
    // Texture loading
    bool load_texture_set(const std::string& style, const std::string& base_path);
    
    // Draw call (simplified)
    void draw_model(const std::string& name);
    
    // Status
    size_t uploaded_count() const { return m_uploaded_count; }
    size_t total_count() const { return m_manifest.count; }
    
    // Hot-reload support
    void reload_manifest();
    
private:
    BlenderManifest m_manifest;
    std::string m_export_dir;
    size_t m_uploaded_count = 0;
    
    // Parse glTF binary (GLB) — simplified, real impl would use tinygltf
    bool parse_glb(const std::string& path, BlenderModel& out_model);
    
    // Upload mesh data to GPU (OpenGL/Vulkan)
    bool upload_mesh_data(BlenderModel& model,
                          const std::vector<float>& vertices,
                          const std::vector<uint32_t>& indices);
    
    // Load image file into GPU texture
    uint32_t load_texture_file(const std::string& path);
};

// ============================================================================
// BLENDER PIPELINE API
// ============================================================================

struct PipelineConfig {
    std::string blender_path = "/usr/bin/blender";
    std::string bridge_script = "";
    std::string export_dir = "~/.twilight-elys/blender_export";
    std::string model_type = "all";
    uint32_t texture_size = 512;
    uint32_t seed = 0;
};

class BlenderPipeline {
public:
    BlenderPipeline() = default;
    
    // Run Blender headless to generate models
    bool run_generation(const PipelineConfig& config);
    
    // Check if Blender is available
    static bool blender_available();
    
    // Get default config
    static PipelineConfig default_config(const std::string& project_dir);
};

} // namespace te::seele
