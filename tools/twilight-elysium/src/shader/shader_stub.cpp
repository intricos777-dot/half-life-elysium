#include "shader_stub.h"
#include <cstdio>
#include <fstream>
#include <filesystem>

namespace te {

bool hot_reload_shader(const std::string& path, ShaderDesc& desc) {
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec) return false;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    desc.bytecode.assign(std::istreambuf_iterator<char>(f), {});
    desc.entry_point = "main";
    return true;
}

} // namespace te
