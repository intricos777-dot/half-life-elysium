#include "shader_pipeline.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(TE_HAS_GL)
#include <GL/glew.h>
#endif

namespace te {

namespace {
constexpr uint32_t kMagic = 0x31444853; // "SHD1"

uint32_t hashSrc(const std::string& s) {
    uint32_t h = 2166136261u;
    for (char c : s) { h ^= (uint8_t)c; h *= 16777619u; }
    return h;
}

bool g_gpu_ready = false;

std::string readFile(const std::string& path, bool& ok) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { ok = false; return {}; }
    std::ostringstream ss;
    ss << f.rdbuf();
    ok = true;
    return ss.str();
}

#if defined(TE_HAS_GL)
uint32_t compileStage(const std::string& src, uint32_t gl_stage) {
    GLuint sh = glCreateShader(gl_stage);
    const char* c = src.c_str();
    glShaderSource(sh, 1, &c, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[ShaderPipeline] compile error: %s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}
#endif

} // namespace

bool ShaderCompiler::compile(const ShaderDesc& desc, std::vector<uint8_t>& out_bytecode) {
    std::vector<uint8_t> out;
    out.reserve(16 + desc.entry_point.size() + desc.bytecode.size());
    auto push32 = [&](uint32_t v) {
        out.push_back((uint8_t)(v & 0xFF));
        out.push_back((uint8_t)((v >> 8) & 0xFF));
        out.push_back((uint8_t)((v >> 16) & 0xFF));
        out.push_back((uint8_t)((v >> 24) & 0xFF));
    };
    push32(kMagic);
    push32((uint32_t)desc.stage);
    push32(hashSrc(desc.entry_point));   // placeholder: refined below

    uint32_t gl_object = 0;
#if defined(TE_HAS_GL)
    if (g_gpu_ready) {
        uint32_t gl_stage;
        switch (desc.stage) {
            case ShaderStage::Vertex:   gl_stage = GL_VERTEX_SHADER;   break;
            case ShaderStage::Fragment: gl_stage = GL_FRAGMENT_SHADER; break;
            default:                    gl_stage = GL_COMPUTE_SHADER;  break;
        }
        gl_object = compileStage(desc.bytecode.empty()
                                     ? std::string("void main(){}\n")
                                     : std::string(desc.bytecode.begin(), desc.bytecode.end()),
                                 gl_stage);
        if (gl_object == 0 && !desc.bytecode.empty()) return false;
    }
#endif
    (void)gl_object;

    // Rebuild header with the true source hash.
    out.clear();
    push32(kMagic);
    push32((uint32_t)desc.stage);
    push32(hashSrc(std::string(desc.bytecode.begin(), desc.bytecode.end())));
    push32(gl_object);
    out.insert(out.end(), desc.bytecode.begin(), desc.bytecode.end());
    out_bytecode = std::move(out);
    return true;
}

bool ShaderCompiler::compile_from_file(const std::string& path, ShaderStage stage,
                                       std::vector<uint8_t>& out_bytecode) {
    bool ok = false;
    std::string src = readFile(path, ok);
    if (!ok) {
        std::fprintf(stderr, "[ShaderPipeline] cannot open %s\n", path.c_str());
        return false;
    }
    ShaderDesc desc;
    desc.stage = stage;
    desc.entry_point = "main";
    desc.bytecode.assign(src.begin(), src.end());
    return compile(desc, out_bytecode);
}

namespace {
std::unordered_map<std::string, std::filesystem::file_time_type>& mtimes() {
    static std::unordered_map<std::string, std::filesystem::file_time_type> m;
    return m;
}
}

bool hot_reload_shader(const std::string& path, ShaderDesc& desc) {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(path, ec);
    if (ec) return false;
    auto& cache = mtimes();
    auto it = cache.find(path);
    if (it != cache.end() && it->second == t) return true; // still fresh
    bool ok = false;
    std::string src = readFile(path, ok);
    if (!ok) return false;
    desc.bytecode.assign(src.begin(), src.end());
    if (desc.bytecode.empty()) {
        const std::string main_only = "void main(){}\n";
        desc.bytecode.assign(main_only.begin(), main_only.end());
    }
    cache[path] = t;
    return true;
}

bool load_shader_stage(const std::string& path, ShaderStage stage,
                       std::vector<uint8_t>& out_bytecode) {
    return ShaderCompiler::compile_from_file(path, stage, out_bytecode);
}

// ---------------------------------------------------------------------------
// ShaderCache (GL program assembly)
// ---------------------------------------------------------------------------
void ShaderCache::setGpuReady(bool ready) { g_gpu_ready = ready; }
bool ShaderCache::gpuReady() { return g_gpu_ready; }

uint32_t ShaderCache::buildProgram(const std::vector<uint8_t>& vs, const std::vector<uint8_t>& fs) {
    (void)vs; (void)fs;
#if defined(TE_HAS_GL)
    if (!g_gpu_ready) return 0;
    std::string vstr(vs.begin(), vs.end());
    std::string fstr(fs.begin(), fs.end());
    GLuint v = compileStage(vstr, GL_VERTEX_SHADER);
    GLuint f = compileStage(fstr, GL_FRAGMENT_SHADER);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[ShaderPipeline] link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
#else
    return 0;
#endif
}

ShaderProgram ShaderCache::getOrBuild(const std::string& vertex_path, const std::string& fragment_path) {
    ShaderProgram sp;
    sp.name = vertex_path + "|" + fragment_path;
#if defined(TE_HAS_GL)
    if (!g_gpu_ready) return sp;
    std::vector<uint8_t> vs, fs;
    ShaderDesc d;
    if (load_shader_stage(vertex_path, ShaderStage::Vertex, vs)) d.bytecode = vs;
    if (load_shader_stage(fragment_path, ShaderStage::Fragment, fs)) d.bytecode = fs;
    sp.program = buildProgram(vs, fs);
#endif
    return sp;
}

} // namespace te