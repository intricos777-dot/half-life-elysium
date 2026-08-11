#include "frame_graph_stub.h"
#include <cstdio>

namespace te {

bool FrameGraph::compile(const RenderGraphDesc& desc) {
    std::printf("[Pipeline] FrameGraph compiled with %zu attachments\n", desc.attachments.size());
    return true;
}

} // namespace te
