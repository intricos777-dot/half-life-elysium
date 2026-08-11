#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace te {

struct AssetMetadata {
    std::string path;
    uint64_t size = 0;
    uint32_t type = 0;
};

class AssetLoader {
public:
    static bool load(const std::string& path, std::vector<uint8_t>& out_data);
};

} // namespace te
