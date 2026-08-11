#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace te {

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
};

enum class TextureFormat : uint32_t {
    RGBA8,
    RGBA16F,
    RGBA32F,
    BC1,
    BC5,
};

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t mip_levels = 1;
    TextureFormat format = TextureFormat::RGBA8;
};

class Texture {
public:
    virtual ~Texture() = default;
    virtual void* get_handle() const = 0;
};

enum class ShaderStage : uint32_t {
    Vertex,
    Fragment,
    Compute,
};

struct ShaderDesc {
    ShaderStage stage;
    std::string entry_point = "main";
    std::vector<uint8_t> bytecode;
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual void* get_handle() const = 0;
};

struct RenderPassDesc {
    std::vector<TextureDesc> color_attachments;
    TextureDesc depth_attachment;
    bool clear = true;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;
    virtual void begin() = 0;
    virtual void end() = 0;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual bool initialize(void* window_handle) = 0;
    virtual void shutdown() = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual Texture* create_texture(const TextureDesc& desc, const void* data) = 0;
    virtual Shader* create_shader(const ShaderDesc& desc) = 0;
    virtual RenderPass* create_render_pass(const RenderPassDesc& desc) = 0;

    virtual void set_vertex_buffer(Texture* buffer, uint32_t slot) = 0;
    virtual void draw(uint32_t vertex_count, uint32_t instance_count = 1) = 0;
};

} // namespace te
