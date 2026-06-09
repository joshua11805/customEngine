#pragma once
#include "TerrainChunk.h"
#include <unordered_map>
#include <vector>
#include <string>

class Shader;

// Manages a grid of TerrainChunks around a focal point (editor or game camera).
// Chunks within m_loadRadius are created; chunks beyond are destroyed.
class TerrainManager
{
public:
    TerrainManager() = default;
    ~TerrainManager();

    // Apply new generation parameters and rebuild all loaded chunks.
    void Regenerate(const TerrainChunk::GenParams& params);

    // Call every frame with the camera world position.
    // Loads/unloads chunks to match the desired radius.
    void Update(const Vector3& cameraPos);

    // Draw all loaded chunks. worldOffset is added to each chunk's model-to-world
    // translation so the owning TerrainActor's position is respected.
    void Draw(SDL_GPUCommandBuffer* commandBuffer,
              SDL_GPURenderPass*   renderPass,
              Shader*              shaderOverride = nullptr,
              const Vector3&       worldOffset    = Vector3::Zero);

    // Destroy all currently loaded chunks (safe to call at any time).
    void UnloadAll();

    // Transfer ownership of all currently loaded chunks out of the manager.
    // The manager's chunk map is emptied; the caller takes ownership of the returned chunks.
    // Use this to stamp the preview into the scene as regular actors.
    std::vector<TerrainChunk*> StealChunks();

    // Settings
    int  GetLoadRadius()  const { return m_loadRadius; }
    void SetLoadRadius(int r)   { m_loadRadius = r; }

    const TerrainChunk::GenParams& GetParams() const { return m_params; }
    void SetParams(const TerrainChunk::GenParams& p) { m_params = p; }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool e) { m_enabled = e; }

    // Shaders used when the manager creates streaming chunks.
    // Must be set before the first Update() that loads chunks.
    void SetShaders(Shader* scene, Shader* game, Shader* sceneWire = nullptr)
        { m_sceneShader = scene; m_gameShader = game; m_sceneWireShader = sceneWire; }

private:
    Shader* m_sceneShader     = nullptr; // non-owning
    Shader* m_gameShader      = nullptr; // non-owning
    Shader* m_sceneWireShader = nullptr; // non-owning
    struct ChunkCoord
    {
        int x, z;
        bool operator==(const ChunkCoord& o) const { return x == o.x && z == o.z; }
    };
    struct ChunkHash
    {
        size_t operator()(const ChunkCoord& c) const
        {
            return std::hash<int>()(c.x) ^ (std::hash<int>()(c.z) * 2654435761u);
        }
    };

    std::unordered_map<ChunkCoord, TerrainChunk*, ChunkHash> m_chunks;
    TerrainChunk::GenParams m_params;
    int  m_loadRadius = 2;
    bool m_enabled    = false;

    TerrainChunk* LoadChunk(int cx, int cz);
};
