#include "pch.h"
#include "TerrainChunk.h"
#include "VertexBuffer.h"
#include "Renderer.h"
#include "Material.h"
#include "Shader.h"

#include "FastNoiseLite.h"

bool TerrainChunk::s_gamePass = false;

Shader* TerrainChunk::PickSceneShader(int mode)
{
    // SceneViewMode: 2=Wireframe, 3=WireframeOnShaded overlay sub-pass
    if ((mode == 2 || mode == 3) && m_terrainShaderWire)
        return m_terrainShaderWire;
    // For all other modes return the terrain scene shader — never let the render loop
    // fall back to the generic SceneLit pipeline which expects VertexPosNormalUV.
    return m_terrainShader;
}

TerrainChunk::TerrainChunk(int chunkX, int chunkZ, const GenParams& params)
    : Actor(nullptr), m_chunkX(chunkX), m_chunkZ(chunkZ)
{
    Generate(params);

    char nameBuf[64];
    SDL_snprintf(nameBuf, sizeof(nameBuf), "Terrain_%d_%d", chunkX, chunkZ);
    SetName(nameBuf);
}

TerrainChunk::~TerrainChunk()
{
    delete m_terrainVB;
    m_terrainVB = nullptr;
}

void TerrainChunk::Generate(const GenParams& params)
{
    delete m_terrainVB;
    m_terrainVB = nullptr;

    int res = params.resolution;
    if (res < 2) res = 2;

    // Vertices are generated in local space (origin at chunk corner = 0,0).
    // The chunk's model-to-world matrix is set to translate to the correct world position,
    // so moving the TerrainActor in the Inspector shifts the whole terrain.
    float worldOriginX = m_chunkX * params.tileSize;
    float worldOriginY = m_chunkZ * params.tileSize;
    float step = params.tileSize / static_cast<float>(res - 1);

    // Store the world offset in the model-to-world matrix so the GPU shader applies it.
    m_POC.c_modelToWorld = Matrix4::CreateTranslation(Vector3(worldOriginX, worldOriginY, 0.f));

    // Set up noise
    FastNoiseLite noise;
    noise.SetSeed(params.seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(params.noiseScale);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(params.octaves);
    noise.SetFractalLacunarity(params.lacunarity);
    noise.SetFractalGain(params.persistence);

    // Generate vertices: terrain lies in the XY plane, height along Z
    std::vector<VertexPosNormalColorUV> verts;
    verts.reserve(res * res);

    // First pass: heights only (needed for normal calculation).
    // Noise is sampled in world space so adjacent chunks tile seamlessly,
    // but vertex XY positions are local (chunk-relative) so the transform matrix works.
    std::vector<float> heights(res * res);
    for (int row = 0; row < res; ++row)
    {
        for (int col = 0; col < res; ++col)
        {
            float wx = worldOriginX + col * step; // world space for noise continuity
            float wy = worldOriginY + row * step;
            float n = noise.GetNoise(wx, wy); // [-1, 1]
            float t = (n + 1.0f) * 0.5f;      // [0, 1]
            heights[row * res + col] = t * params.heightScale;
        }
    }

    // Second pass: build vertices with normals via finite difference
    // dPos/dcol = (step, 0, dH/dcol), dPos/drow = (0, step, dH/drow)
    // normal = cross(dPos/dcol, dPos/drow) = (-dH/dcol*step, -dH/drow*step, step^2) normalised
    auto getH = [&](int col, int row) -> float {
        col = SDL_clamp(col, 0, res - 1);
        row = SDL_clamp(row, 0, res - 1);
        return heights[row * res + col];
    };

    for (int row = 0; row < res; ++row)
    {
        for (int col = 0; col < res; ++col)
        {
            float h  = heights[row * res + col];
            // Local position: (col*step, row*step) within this chunk, Z = height
            float lx = col * step;
            float ly = row * step;

            float hL = getH(col - 1, row);
            float hR = getH(col + 1, row);
            float hD = getH(col, row - 1);
            float hU = getH(col, row + 1);

            // Normal in XY-ground, Z-up space
            Vector3 normal(hL - hR, hD - hU, 2.0f * step);
            normal.Normalize();

            float t = h / params.heightScale;
            Color4 col4 = HeightColor(t);

            float uvX = static_cast<float>(col) / static_cast<float>(res - 1);
            float uvY = static_cast<float>(row) / static_cast<float>(res - 1);

            // Local space position — world offset is in the model-to-world matrix
            verts.push_back({ Vector3(lx, ly, h), normal, col4, Vector2(uvX, uvY) });
        }
    }

    // Generate indices
    std::vector<uint32_t> indices;
    int quads = res - 1;
    indices.reserve(quads * quads * 6);
    for (int z = 0; z < quads; ++z)
    {
        for (int x = 0; x < quads; ++x)
        {
            uint32_t tl = z * res + x;
            uint32_t tr = tl + 1;
            uint32_t bl = (z + 1) * res + x;
            uint32_t br = bl + 1;

            // CCW winding when viewed from above (+Z), so the top face is the front face
            indices.push_back(tl); indices.push_back(tr); indices.push_back(bl);
            indices.push_back(tr); indices.push_back(br); indices.push_back(bl);
        }
    }

    m_terrainVB = new VertexBuffer(
        Renderer::Get(),
        verts.data(),   static_cast<uint32_t>(verts.size()   * sizeof(VertexPosNormalColorUV)),
        indices.data(), static_cast<uint32_t>(indices.size()), sizeof(uint32_t)
    );
}

void TerrainChunk::Draw(SDL_GPUCommandBuffer* commandBuffer,
                        SDL_GPURenderPass*   renderPass,
                        Shader*              shaderOverride)
{
    // Use explicit override when provided (e.g. wireframe scene mode from the render loop).
    // Fall back to the internal terrain shaders based on the current pass.
    Shader* shader = shaderOverride
        ? shaderOverride
        : (s_gamePass ? m_terrainShaderGame : m_terrainShader);
    if (!m_terrainVB || !shader) return;

    SDL_PushGPUVertexUniformData(
        commandBuffer,
        Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_RENDEROBJ,
        &m_POC, sizeof(m_POC));

    shader->SetActive(renderPass);
    m_terrainVB->Draw(commandBuffer, renderPass);
}

// Height-based colour bands (water → sand → grass → rock → snow)
Color4 TerrainChunk::HeightColor(float t)
{
    struct Band { float threshold; Color4 color; };
    static const Band bands[] = {
        { 0.10f, { 0.10f, 0.25f, 0.60f, 1.f } }, // deep water
        { 0.18f, { 0.20f, 0.45f, 0.75f, 1.f } }, // shallow water
        { 0.22f, { 0.80f, 0.75f, 0.50f, 1.f } }, // sand
        { 0.40f, { 0.25f, 0.55f, 0.20f, 1.f } }, // grass
        { 0.60f, { 0.35f, 0.65f, 0.25f, 1.f } }, // highlands
        { 0.75f, { 0.45f, 0.38f, 0.28f, 1.f } }, // rock
        { 0.88f, { 0.55f, 0.48f, 0.38f, 1.f } }, // high rock
        { 1.00f, { 0.90f, 0.92f, 0.95f, 1.f } }, // snow
    };

    for (const Band& b : bands)
    {
        if (t <= b.threshold)
            return b.color;
    }
    return bands[SDL_arraysize(bands) - 1].color;
}
