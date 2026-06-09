#include "pch.h"
#include "TerrainManager.h"
#include "Shader.h"

TerrainManager::~TerrainManager()
{
    UnloadAll();
}

void TerrainManager::Regenerate(const TerrainChunk::GenParams& params)
{
    m_params = params;
    UnloadAll();
}

void TerrainManager::Update(const Vector3& cameraPos)
{
    if (!m_enabled) return;

    float tileSize = m_params.tileSize;

    // Terrain lies in the XY plane (Z is up in this engine).
    // Determine which chunk the camera is over using X and Y.
    int camCX = static_cast<int>(SDL_floorf(cameraPos.x / tileSize));
    int camCZ = static_cast<int>(SDL_floorf(cameraPos.y / tileSize));

    // Collect which chunks should be loaded
    std::vector<ChunkCoord> desired;
    for (int dz = -m_loadRadius; dz <= m_loadRadius; ++dz)
        for (int dx = -m_loadRadius; dx <= m_loadRadius; ++dx)
            desired.push_back({ camCX + dx, camCZ + dz });

    // Unload chunks no longer in range
    std::vector<ChunkCoord> toRemove;
    for (auto& [coord, chunk] : m_chunks)
    {
        bool found = false;
        for (const auto& d : desired)
            if (d == coord) { found = true; break; }
        if (!found)
            toRemove.push_back(coord);
    }
    for (const ChunkCoord& c : toRemove)
    {
        delete m_chunks[c];
        m_chunks.erase(c);
    }

    // Load new chunks
    for (const ChunkCoord& d : desired)
    {
        if (m_chunks.find(d) == m_chunks.end())
        {
            m_chunks[d] = LoadChunk(d.x, d.z);
        }
    }
}

void TerrainManager::Draw(SDL_GPUCommandBuffer* commandBuffer,
                          SDL_GPURenderPass*   renderPass,
                          Shader*              shaderOverride,
                          const Vector3&       worldOffset)
{
    if (!m_enabled) return;
    bool hasOffset = (worldOffset.x != 0.f || worldOffset.y != 0.f || worldOffset.z != 0.f);
    for (auto& [coord, chunk] : m_chunks)
    {
        if (hasOffset)
        {
            Matrix4 orig = chunk->GetTransform();
            Vector3 t    = orig.GetTranslation();
            chunk->SetTransform(Matrix4::CreateTranslation(t + worldOffset));
            chunk->Draw(commandBuffer, renderPass, shaderOverride);
            chunk->SetTransform(orig);
        }
        else
        {
            chunk->Draw(commandBuffer, renderPass, shaderOverride);
        }
    }
}

void TerrainManager::UnloadAll()
{
    for (auto& [coord, chunk] : m_chunks)
        delete chunk;
    m_chunks.clear();
}

std::vector<TerrainChunk*> TerrainManager::StealChunks()
{
    std::vector<TerrainChunk*> out;
    out.reserve(m_chunks.size());
    for (auto& [coord, chunk] : m_chunks)
        out.push_back(chunk);
    m_chunks.clear(); // manager no longer owns them
    return out;
}

TerrainChunk* TerrainManager::LoadChunk(int cx, int cz)
{
    TerrainChunk* chunk = new TerrainChunk(cx, cz, m_params);
    chunk->SetTerrainShaders(m_sceneShader, m_gameShader, m_sceneWireShader);
    return chunk;
}
