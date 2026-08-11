#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace te {

enum class ShaderStage : uint32_t { Vertex, Fragment, Compute };

struct ShaderDesc {
    ShaderStage stage;
    std::string entry_point;
    std::vector<uint8_t> bytecode;
};

class ShaderCompiler {
public:
    static bool compile(const ShaderDesc& desc, std::vector<uint8_t>& out_bytecode) {
        out_bytecode = desc.bytecode;
        return true;
    }
};

bool hot_reload_shader(const std::string& path, ShaderDesc& desc);

} // namespace te
