#pragma once
#include <cstdint>
#include <vector>

namespace te {

struct AttachmentDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t format = 0;
};

struct RenderGraphDesc {
    std::vector<AttachmentDesc> attachments;
};

class FrameGraph {
public:
    static bool compile(const RenderGraphDesc& desc);
};

} // namespace te
