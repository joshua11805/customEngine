#include "../pch.h"
#include "GizmoRenderer.h"
#include "../Renderer.h"
#include "../Shader.h"
#include "../Texture.h"
#include "../Actor.h"
#include <cfloat>

static constexpr uint32_t k_maxVerts = 4096;
static constexpr uint32_t k_maxInds  = 8192;

bool GizmoRenderer::Init(Renderer* renderer, Texture* sceneRT)
{
    // Build the gizmo line shader — vertex-color, line primitives rendered as triangles
    // (SDL_GPU has no native line primitive mode on all backends, so we draw thin quads
    //  — but here we just use FILL triangles and narrow geometry).
    // We actually use LINE topology via SDL_GPU_PRIMITIVETYPE_LINELIST in SetActive.
    // The pipeline is created with LINE primitive type via a custom path below.

    m_shader = new Shader(renderer, "Shaders/GizmoLine.hlsl");

    SDL_GPUVertexAttribute attribs[] = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(GizmoVertex, pos) },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(GizmoVertex, r)   },
    };

    m_shader->SetColorTarget(sceneRT);
    m_shader->SetZWrite(false);
    m_shader->SetZTest(false);
    m_shader->SetCullMode(SDL_GPU_CULLMODE_NONE);
    m_shader->SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_LINELIST);
    m_shader->CreatePipeline(attribs, 2, sizeof(GizmoVertex));

    // Pre-allocate GPU buffers at max capacity with zeroed data (re-uploaded each frame)
    std::vector<GizmoVertex> zeroVerts(k_maxVerts, GizmoVertex{});
    std::vector<uint16_t>    zeroInds (k_maxInds,  0);
    m_vertexBuffer = renderer->CreateBuffer(zeroVerts.data(),
        k_maxVerts * sizeof(GizmoVertex), SDL_GPU_BUFFERUSAGE_VERTEX);
    m_indexBuffer  = renderer->CreateBuffer(zeroInds.data(),
        k_maxInds  * sizeof(uint16_t),   SDL_GPU_BUFFERUSAGE_INDEX);
    m_allocatedVerts = k_maxVerts;
    m_allocatedInds  = k_maxInds;

    return m_vertexBuffer && m_indexBuffer && m_shader;
}

void GizmoRenderer::Shutdown()
{
    if (m_vertexBuffer)
    {
        SDL_ReleaseGPUBuffer(Renderer::Get()->GetDevice(), m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }
    if (m_indexBuffer)
    {
        SDL_ReleaseGPUBuffer(Renderer::Get()->GetDevice(), m_indexBuffer);
        m_indexBuffer = nullptr;
    }
    delete m_shader;
    m_shader = nullptr;
}

void GizmoRenderer::BeginFrame()
{
    m_vertices.clear();
    m_indices.clear();
}

// ─── private helpers ──────────────────────────────────────────────────────────

void GizmoRenderer::AddLine(Vector3 a, Vector3 b, float r, float g, float b_, float a_)
{
    if (m_vertices.size() + 2 > k_maxVerts || m_indices.size() + 2 > k_maxInds)
        return;

    uint16_t base = static_cast<uint16_t>(m_vertices.size());
    m_vertices.push_back({ a, r, g, b_, a_ });
    m_vertices.push_back({ b, r, g, b_, a_ });
    m_indices.push_back(base);
    m_indices.push_back(base + 1);
}

void GizmoRenderer::AddArrow(Vector3 origin, Vector3 dir, float len, float r, float g, float b_)
{
    Vector3 tip = origin + dir * len;
    AddLine(origin, tip, r, g, b_, 1.f);

    // Small cross at the tip so arrows are visible even as thin lines
    // Find two perpendicular vectors to 'dir'
    Vector3 perp;
    if (fabsf(dir.x) < 0.9f)
        perp = Vector3::Cross(dir, Vector3(1, 0, 0));
    else
        perp = Vector3::Cross(dir, Vector3(0, 0, 1));
    perp = Vector3::Normalize(perp);
    Vector3 perp2 = Vector3::Cross(dir, perp);
    perp2 = Vector3::Normalize(perp2);

    float capLen = len * 0.18f;
    float capBack = len * 0.12f;
    Vector3 capBase = tip - dir * capBack;
    AddLine(tip, capBase + perp  * capLen, r, g, b_, 1.f);
    AddLine(tip, capBase - perp  * capLen, r, g, b_, 1.f);
    AddLine(tip, capBase + perp2 * capLen, r, g, b_, 1.f);
    AddLine(tip, capBase - perp2 * capLen, r, g, b_, 1.f);
}

void GizmoRenderer::AddCircle(Vector3 center, Vector3 normal, float radius,
                               float r, float g, float b_, int segments)
{
    // Build two axes perpendicular to 'normal'
    Vector3 axisA;
    if (fabsf(normal.x) < 0.9f)
        axisA = Vector3::Normalize(Vector3::Cross(normal, Vector3(1, 0, 0)));
    else
        axisA = Vector3::Normalize(Vector3::Cross(normal, Vector3(0, 0, 1)));
    Vector3 axisB = Vector3::Normalize(Vector3::Cross(normal, axisA));

    float step = Math::TwoPi / static_cast<float>(segments);
    Vector3 prev = center + axisA * radius;
    for (int i = 1; i <= segments; ++i)
    {
        float angle = i * step;
        Vector3 cur = center + axisA * (cosf(angle) * radius) + axisB * (sinf(angle) * radius);
        AddLine(prev, cur, r, g, b_, 1.f);
        prev = cur;
    }
}

// ─── public geometry builders ─────────────────────────────────────────────────

void GizmoRenderer::AddTranslateGizmo(const Matrix4& transform, float handleSize)
{
    Vector3 pos = transform.GetTranslation();

    // Extract world-space axes from the transform (ignoring scale)
    Matrix4 rotOnly = transform;
    // Zero out translation
    rotOnly.mat[3][0] = rotOnly.mat[3][1] = rotOnly.mat[3][2] = 0.f;
    Vector3 xAxis = Vector3::Normalize(rotOnly.GetXAxis());
    Vector3 yAxis = Vector3::Normalize(rotOnly.GetYAxis());
    Vector3 zAxis = Vector3::Normalize(rotOnly.GetZAxis());

    AddArrow(pos, xAxis, handleSize, 1.f, 0.1f, 0.1f); // X = red
    AddArrow(pos, yAxis, handleSize, 0.1f, 1.f, 0.1f); // Y = green
    AddArrow(pos, zAxis, handleSize, 0.1f, 0.1f, 1.f); // Z = blue
}

void GizmoRenderer::AddRotateGizmo(const Matrix4& transform, float handleSize)
{
    Vector3 pos = transform.GetTranslation();

    Matrix4 rotOnly = transform;
    rotOnly.mat[3][0] = rotOnly.mat[3][1] = rotOnly.mat[3][2] = 0.f;
    Vector3 xAxis = Vector3::Normalize(rotOnly.GetXAxis());
    Vector3 yAxis = Vector3::Normalize(rotOnly.GetYAxis());
    Vector3 zAxis = Vector3::Normalize(rotOnly.GetZAxis());

    AddCircle(pos, xAxis, handleSize, 1.f, 0.1f, 0.1f); // X ring = red
    AddCircle(pos, yAxis, handleSize, 0.1f, 1.f, 0.1f); // Y ring = green
    AddCircle(pos, zAxis, handleSize, 0.1f, 0.1f, 1.f); // Z ring = blue
}

void GizmoRenderer::AddScaleGizmo(const Matrix4& transform, float handleSize)
{
    Vector3 pos = transform.GetTranslation();

    Matrix4 rotOnly = transform;
    rotOnly.mat[3][0] = rotOnly.mat[3][1] = rotOnly.mat[3][2] = 0.f;
    Vector3 xAxis = Vector3::Normalize(rotOnly.GetXAxis());
    Vector3 yAxis = Vector3::Normalize(rotOnly.GetYAxis());
    Vector3 zAxis = Vector3::Normalize(rotOnly.GetZAxis());

    // Arrows like translate but with a square cap instead of pyramid
    float capSize = handleSize * 0.10f;
    auto AddScaleAxis = [&](Vector3 axis, float r, float g, float b_)
    {
        Vector3 tip = pos + axis * handleSize;
        AddLine(pos, tip, r, g, b_, 1.f);
        // Draw a small cube marker at the tip using 4 cross lines
        Vector3 perp;
        if (fabsf(axis.x) < 0.9f) perp = Vector3::Normalize(Vector3::Cross(axis, Vector3(1,0,0)));
        else                       perp = Vector3::Normalize(Vector3::Cross(axis, Vector3(0,0,1)));
        Vector3 perp2 = Vector3::Normalize(Vector3::Cross(axis, perp));
        // 4 edges of cube face
        Vector3 corners[4] = {
            tip + (perp + perp2) * capSize,
            tip + (perp2 - perp) * capSize,
            tip - (perp + perp2) * capSize,
            tip + (perp - perp2) * capSize,
        };
        for (int i = 0; i < 4; ++i)
            AddLine(corners[i], corners[(i+1)%4], r, g, b_, 1.f);
    };

    AddScaleAxis(xAxis, 1.f, 0.1f, 0.1f);
    AddScaleAxis(yAxis, 0.1f, 1.f, 0.1f);
    AddScaleAxis(zAxis, 0.1f, 0.1f, 1.f);
}

void GizmoRenderer::AddSelectionBox(const Matrix4& transform, Vector3 extents)
{
    // 8 corners of a box in object space, scaled by extents, transformed to world space
    const float ex = extents.x, ey = extents.y, ez = extents.z;
    const Vector3 localCorners[8] = {
        Vector3(-ex,-ey,-ez), Vector3( ex,-ey,-ez), Vector3( ex, ey,-ez), Vector3(-ex, ey,-ez),
        Vector3(-ex,-ey, ez), Vector3( ex,-ey, ez), Vector3( ex, ey, ez), Vector3(-ex, ey, ez),
    };

    Vector3 wc[8];
    for (int i = 0; i < 8; ++i)
        wc[i] = Matrix4::Transform(localCorners[i], transform);

    // 12 edges of the box
    static const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // bottom face
        {4,5},{5,6},{6,7},{7,4}, // top face
        {0,4},{1,5},{2,6},{3,7}, // vertical edges
    };

    float r = 1.f, g = 0.75f, b_ = 0.f; // orange selection color
    for (auto& e : edges)
        AddLine(wc[e[0]], wc[e[1]], r, g, b_, 1.f);
}

// ─── GPU upload & draw ────────────────────────────────────────────────────────

void GizmoRenderer::Upload(Renderer* renderer)
{
    if (m_vertices.empty()) return;

    // Grow buffers if needed (shouldn't happen with k_max guards, but be safe)
    uint32_t vSize = static_cast<uint32_t>(m_vertices.size() * sizeof(GizmoVertex));
    uint32_t iSize = static_cast<uint32_t>(m_indices.size()  * sizeof(uint16_t));

    renderer->UploadBuffer(m_vertexBuffer, m_vertices.data(), vSize);
    renderer->UploadBuffer(m_indexBuffer,  m_indices.data(),  iSize);
}

void GizmoRenderer::Draw(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass)
{
    if (m_vertices.empty() || !m_shader) return;

    // Push identity model-to-world (geometry is already in world space)
    Actor::PerObjectConstants poc;
    poc.c_modelToWorld = Matrix4::Identity;
    SDL_PushGPUVertexUniformData(cmd, Renderer::CONSTANT_VERTEX_RENDEROBJ, &poc, sizeof(poc));

    m_shader->SetActive(pass);

    const SDL_GPUBufferBinding vb = { m_vertexBuffer, 0 };
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);

    const SDL_GPUBufferBinding ib = { m_indexBuffer, 0 };
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(pass, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int GizmoRenderer::HitTestTranslateGizmo(const Matrix4& transform, float handleSize,
                                          Vector3 rayOrigin, Vector3 rayDir) const
{
    Vector3 pos = transform.GetTranslation();

    Matrix4 rotOnly = transform;
    rotOnly.mat[3][0] = rotOnly.mat[3][1] = rotOnly.mat[3][2] = 0.f;
    Vector3 axes[3] = {
        Vector3::Normalize(rotOnly.GetXAxis()),
        Vector3::Normalize(rotOnly.GetYAxis()),
        Vector3::Normalize(rotOnly.GetZAxis()),
    };

    // Screen-space hit test: find the closest point on each axis arrow to the ray,
    // then measure how far that world point is from the ray in units of ray-distance.
    // This gives a pick tolerance that scales with distance automatically.
    // Threshold: tan(pickAngle) where pickAngle ~ 4 degrees gives good feel.
    const float tanPickAngle = 0.07f; // ~4 degrees half-angle cone around the ray

    int   bestAxis = 0;
    float bestDist = FLT_MAX;

    for (int i = 0; i < 3; ++i)
    {
        Vector3 ax = axes[i];

        Vector3 w     = rayOrigin - pos;
        float   b     = Vector3::Dot(w,      rayDir);
        float   d     = Vector3::Dot(w,      ax);
        float   e     = Vector3::Dot(rayDir, ax);
        float   denom = 1.f - e * e;
        if (fabsf(denom) < 1e-6f) continue;

        float sc = (e * d - b) / denom;  // param along axis
        sc = Math::Clamp(sc, 0.f, handleSize);

        // Recompute tc for the clamped sc: project the clamped axis point onto the ray
        Vector3 closestOnAxis = pos + ax * sc;
        Vector3 toAxis        = closestOnAxis - rayOrigin;
        float   tc            = Math::Max(0.f, Vector3::Dot(toAxis, rayDir));

        Vector3 closestOnRay  = rayOrigin + rayDir * tc;
        float   worldDist     = (closestOnAxis - closestOnRay).Length();

        // Angular miss: world distance / ray distance — distance-independent pick tolerance
        float angularDist = (tc > 1e-3f) ? (worldDist / tc) : worldDist;

        if (angularDist < tanPickAngle && angularDist < bestDist)
        {
            bestDist = angularDist;
            bestAxis = i + 1;
        }
    }

    return bestAxis;
}
