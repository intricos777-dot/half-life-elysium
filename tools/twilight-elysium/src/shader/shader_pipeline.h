#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "renderer/renderer.h"   // te::ShaderStage, te::ShaderDesc (house API)

namespace te {

// ============================================================================
// SHADER PIPELINE — real replacement for shader_stub.
// Compiles GLSL stages against the engine's ShaderDesc model, caches programs
// by source hash, and supports file-based hot reload.
// ============================================================================

class ShaderCompiler {
public:
    // Compile `desc` → bytecode. Semantic (GL): bytecode begins with the
    // magic "TE_SHD1" followed by [stage:u32][sourceHash:u32][glObjectId:u32]
    // then the canonical source text (for tooling/caching).
    static bool compile(const ShaderDesc& desc, std::vector<uint8_t>& out_bytecode);

    // Read `path`, compile as `stage`.
    static bool compile_from_file(const std::string& path, ShaderStage stage,
                                  std::vector<uint8_t>& out_bytecode);
};

// Compile `path` (if newer than the last compile) into `desc.bytecode`.
// Returns true when a (re)compile happened or the cached code is still fresh.
bool hot_reload_shader(const std::string& path, ShaderDesc& desc);

// Load + compile a shader stage from file, storing the result in out_bytecode.
bool load_shader_stage(const std::string& path, ShaderStage stage,
                       std::vector<uint8_t>& out_bytecode);

// GL program cache (real GL paths; no-ops without a context).
struct ShaderProgram {
    uint32_t program = 0;
    std::string name;
};

class ShaderCache {
public:
    ShaderProgram getOrBuild(const std::string& vertex_path, const std::string& fragment_path);

    // Mark that a GL context is live (call after glewInit).
    static void setGpuReady(bool ready);
    static bool gpuReady();

private:
    uint32_t buildProgram(const std::vector<uint8_t>& vs, const std::vector<uint8_t>& fs);
};

} // namespace te