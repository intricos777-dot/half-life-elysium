#include "asset_stub.h"
#include <cstdio>

namespace te {

bool AssetLoader::load(const std::string& path, std::vector<uint8_t>& out_data) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    if (sz < 0) { std::fclose(f); return false; }
    std::rewind(f);
    out_data.resize((size_t)sz);
    size_t rd = std::fread(out_data.data(), 1, out_data.size(), f);
    std::fclose(f);
    return rd == out_data.size();
}

} // namespace te
