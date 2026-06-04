#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "../EngineMath.h"
#include "../VertexFormat.h"

class Renderer;
class Shader;
class Texture;

// Collects UI quads submitted each frame, batches them by texture, and
// issues the GPU draw calls. One UIRenderer is owned by a UICanvas.
class UIRenderer {
public:
    // Maximum quads per frame — raise if needed.
    static constexpr uint32_t MAX_QUADS = 4096;

    UIRenderer()  = default;
    ~UIRenderer() = default;

    bool Init(Renderer* renderer);
    void Shutdown();

    // Reset per-frame state. Called by UICanvas::Prepare().
    void BeginFrame();

    // Submit an axis-aligned quad in screen pixels (top-left origin, Y-down).
    // texture == nullptr uses the internal white 1x1 fallback.
    void SubmitQuad(float x, float y, float w, float h,
                    float u0, float v0, float u1, float v1,
                    const Color4& color,
                    const Texture* texture);

    // Convenience: solid-color rectangle.
    void SubmitColoredQuad(float x, float y, float w, float h, const Color4& color);

    // Build CPU geometry and upload to GPU. Call BEFORE BeginCommandBuffer each frame.
    void Upload(Renderer* renderer);

    // Bind resources and issue draw calls. Call inside a render pass.
    void Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                Shader* shader, int screenW, int screenH);

    const Texture* GetWhiteTexture() const { return m_whiteTexture; }

private:
    struct QuadData {
        float x, y, w, h;
        float u0, v0, u1, v1;
        Color4 color;
        const Texture* texture;
    };

    struct DrawBatch {
        const Texture* texture;
        uint32_t startIndex;  // first index in the static index buffer
        uint32_t indexCount;
    };

    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer  = nullptr;
    Texture*       m_whiteTexture = nullptr;

    std::vector<QuadData>  m_quads;
    std::vector<DrawBatch> m_batches;
    std::vector<VertexUI>  m_vertices;
};
