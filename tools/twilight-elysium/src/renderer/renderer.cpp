#include "renderer.h"
#include <cstdio>
#include <memory>
#include <vector>

namespace te {

class DummyTexture final : public Texture {
public:
    void* get_handle() const override { return nullptr; }
};

class DummyShader final : public Shader {
public:
    void* get_handle() const override { return nullptr; }
};

class DummyRenderPass final : public RenderPass {
public:
    void begin() override { std::printf("[Renderer] RenderPass begin\n"); }
    void end() override { std::printf("[Renderer] RenderPass end\n"); }
};

class DummyRenderer final : public Renderer {
public:
    bool initialize(void* window_handle) override {
        (void)window_handle;
        std::printf("[Renderer] Dummy backend initialized\n");
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        if (m_initialized) {
            std::printf("[Renderer] Dummy backend shutdown\n");
            m_initialized = false;
        }
    }

    void begin_frame() override {
        if (!m_initialized) return;
        // Frame begin stub
    }

    void end_frame() override {
        if (!m_initialized) return;
        // Frame end stub
    }

    Texture* create_texture(const TextureDesc& desc, const void* data) override {
        (void)desc; (void)data;
        if (!m_initialized) return nullptr;
        std::printf("[Renderer] Dummy texture created\n");
        m_textures.push_back(std::make_unique<DummyTexture>());
        return m_textures.back().get();
    }

    Shader* create_shader(const ShaderDesc& desc) override {
        (void)desc;
        if (!m_initialized) return nullptr;
        std::printf("[Renderer] Dummy shader created\n");
        m_shaders.push_back(std::make_unique<DummyShader>());
        return m_shaders.back().get();
    }

    RenderPass* create_render_pass(const RenderPassDesc& desc) override {
        (void)desc;
        if (!m_initialized) return nullptr;
        std::printf("[Renderer] Dummy render pass created\n");
        m_passes.push_back(std::make_unique<DummyRenderPass>());
        return m_passes.back().get();
    }

    void set_vertex_buffer(Texture* buffer, uint32_t slot) override {
        (void)buffer; (void)slot;
        if (!m_initialized) return;
        // Vertex buffer bind stub
    }

    void draw(uint32_t vertex_count, uint32_t instance_count) override {
        if (!m_initialized) return;
        std::printf("[Renderer] Dummy draw vertices=%u instances=%u\n",
            vertex_count, instance_count);
    }

private:
    bool m_initialized = false;
    std::vector<std::unique_ptr<DummyTexture>> m_textures;
    std::vector<std::unique_ptr<DummyShader>> m_shaders;
    std::vector<std::unique_ptr<DummyRenderPass>> m_passes;
};

} // namespace te
