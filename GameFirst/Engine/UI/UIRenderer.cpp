#include "../pch.h"
#include "UIRenderer.h"
#include "../Renderer.h"
#include "../Shader.h"
#include "../Texture.h"
#include "../VertexFormat.h"
#include <algorithm>

bool UIRenderer::Init(Renderer* renderer)
{
    // Pre-generate a static index buffer for MAX_QUADS quads.
    // Each quad uses 4 vertices (TL, TR, BR, BL) and 6 indices: 0,1,2, 0,2,3.
    std::vector<uint16_t> indices;
    indices.reserve(MAX_QUADS * 6);
    for (uint32_t i = 0; i < MAX_QUADS; ++i)
    {
        uint16_t b = static_cast<uint16_t>(i * 4);
        indices.push_back(b + 0); indices.push_back(b + 1); indices.push_back(b + 2);
        indices.push_back(b + 0); indices.push_back(b + 2); indices.push_back(b + 3);
    }
    m_indexBuffer = renderer->CreateBuffer(
        indices.data(),
        static_cast<uint32_t>(indices.size() * sizeof(uint16_t)),
        SDL_GPU_BUFFERUSAGE_INDEX);
    if (!m_indexBuffer)
    {
        SDL_Log("UIRenderer::Init — failed to create index buffer");
        return false;
    }

    // Allocate a dynamic vertex buffer (content refreshed each frame via Upload).
    uint32_t vbSize = MAX_QUADS * 4 * sizeof(VertexUI);
    std::vector<VertexUI> empty(MAX_QUADS * 4);
    m_vertexBuffer = renderer->CreateBuffer(
        empty.data(), vbSize, SDL_GPU_BUFFERUSAGE_VERTEX);
    if (!m_vertexBuffer)
    {
        SDL_Log("UIRenderer::Init — failed to create vertex buffer");
        return false;
    }

    // White 1x1 fallback texture for solid-color panels.
    m_whiteTexture = new Texture();
    if (!m_whiteTexture->CreateSolidColor(255, 255, 255, 255, "__UIWhite__"))
    {
        SDL_Log("UIRenderer::Init — failed to create white texture");
        delete m_whiteTexture;
        m_whiteTexture = nullptr;
        return false;
    }

    m_quads.reserve(MAX_QUADS);
    m_vertices.reserve(MAX_QUADS * 4);
    m_batches.reserve(32);
    return true;
}

void UIRenderer::Shutdown()
{
    delete m_whiteTexture;
    m_whiteTexture = nullptr;

    auto* dev = Renderer::Get()->GetDevice();
    if (m_vertexBuffer) { SDL_ReleaseGPUBuffer(dev, m_vertexBuffer); m_vertexBuffer = nullptr; }
    if (m_indexBuffer)  { SDL_ReleaseGPUBuffer(dev, m_indexBuffer);  m_indexBuffer  = nullptr; }
}

void UIRenderer::BeginFrame()
{
    m_quads.clear();
    m_batches.clear();
    m_vertices.clear();
}

void UIRenderer::SubmitQuad(float x, float y, float w, float h,
                             float u0, float v0, float u1, float v1,
                             const Color4& color,
                             const Texture* texture)
{
    if (m_quads.size() >= MAX_QUADS) return;
    const Texture* tex = texture ? texture : m_whiteTexture;
    m_quads.push_back({x, y, w, h, u0, v0, u1, v1, color, tex});
}

void UIRenderer::SubmitColoredQuad(float x, float y, float w, float h, const Color4& color)
{
    SubmitQuad(x, y, w, h, 0.f, 0.f, 1.f, 1.f, color, m_whiteTexture);
}

void UIRenderer::Upload(Renderer* renderer)
{
    if (m_quads.empty()) return;

    // Sort by texture pointer to group same-texture quads into one draw call.
    std::stable_sort(m_quads.begin(), m_quads.end(),
        [](const QuadData& a, const QuadData& b){ return a.texture < b.texture; });

    m_vertices.clear();
    m_batches.clear();

    const Texture* curTex   = nullptr;
    uint32_t       batchQuadStart = 0;
    uint32_t       batchCount    = 0;

    for (const QuadData& q : m_quads)
    {
        if (q.texture != curTex)
        {
            if (batchCount > 0)
                m_batches.push_back({curTex, batchQuadStart * 6, batchCount * 6});
            curTex        = q.texture;
            batchQuadStart = static_cast<uint32_t>(m_vertices.size() / 4);
            batchCount    = 0;
        }

        // Vertex order TL, BL, BR, TR with indices 0,1,2 / 0,2,3.
        // The Y-flip in the vertex shader (screen→NDC) reverses handedness, so
        // TL→TR→BR would be CW (back-face culled). TL→BL→BR is CCW — front-facing.
        VertexUI verts[4] = {
            { Vector2(q.x,       q.y      ), Vector2(q.u0, q.v0), q.color },  // 0: TL
            { Vector2(q.x,       q.y + q.h), Vector2(q.u0, q.v1), q.color },  // 1: BL
            { Vector2(q.x + q.w, q.y + q.h), Vector2(q.u1, q.v1), q.color },  // 2: BR
            { Vector2(q.x + q.w, q.y      ), Vector2(q.u1, q.v0), q.color },  // 3: TR
        };
        for (auto& v : verts) m_vertices.push_back(v);
        ++batchCount;
    }
    if (batchCount > 0)
        m_batches.push_back({curTex, batchQuadStart * 6, batchCount * 6});

    renderer->UploadBuffer(
        m_vertexBuffer,
        m_vertices.data(),
        static_cast<uint32_t>(m_vertices.size() * sizeof(VertexUI)));
}

void UIRenderer::Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                         Shader* shader, int screenW, int screenH)
{
    if (m_batches.empty() || !shader) return;

    // Bind pipeline and vertex/index buffers once.
    shader->SetActive(pass);

    SDL_GPUBufferBinding vbBind = {m_vertexBuffer, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

    SDL_GPUBufferBinding ibBind = {m_indexBuffer, 0};
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Push screen dimensions to vertex shader (slot 0).
    struct UIScreenConst { float w, h, _p0, _p1; };
    UIScreenConst sc = { static_cast<float>(screenW), static_cast<float>(screenH), 0.f, 0.f };
    SDL_PushGPUVertexUniformData(cmd, 0, &sc, sizeof(sc));

    // One draw call per texture batch.
    for (const DrawBatch& batch : m_batches)
    {
        if (batch.texture)
            batch.texture->SetActive(pass, 0);

        SDL_DrawGPUIndexedPrimitives(pass,
            batch.indexCount,   // numIndices
            1,                  // numInstances
            batch.startIndex,   // firstIndex
            0,                  // vertexOffset
            0);                 // firstInstance
    }
}
