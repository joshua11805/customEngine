#pragma once
#include "Actor.h"
#include "TerrainChunk.h"

class TerrainManager;
class Shader;

// A scene actor that wraps a TerrainManager.
// Shows as a single "Terrain" entry in the Hierarchy.
// Streams chunks around the active camera each frame.
// Serialised as type="terrain" in the level file.
class TerrainActor : public Actor
{
public:
    TerrainActor();
    ~TerrainActor() override;

    // Called by the render loop each frame before Draw.
    // cameraPos must be in world space.
    void UpdateStreaming(const Vector3& cameraPos);

    void Draw(SDL_GPUCommandBuffer* commandBuffer,
              SDL_GPURenderPass*   renderPass,
              Shader*              shaderOverride = nullptr) override;

    TerrainManager* GetManager() const { return m_manager; }

    // Must be called after shaders are loaded so scene-mode switching works.
    void SetShaders(Shader* scene, Shader* game, Shader* sceneWire);

    // Returns the terrain-compatible scene shader for the given mode so the render
    // loop never falls back to the generic VertexPosNormalUV pipeline.
    Shader* PickSceneShader(int mode) override;

    // Sample terrain height at a world XY position (accounts for actor offset).
    // Returns 0 if no manager is present.
    float GetHeightAt(float worldX, float worldY) const;

    // Convenience forwarded to manager
    const TerrainChunk::GenParams& GetParams() const;
    void SetParams(const TerrainChunk::GenParams& p);

private:
    TerrainManager* m_manager    = nullptr; // owned
    Shader*         m_sceneShader = nullptr; // non-owning
    Shader*         m_gameShader  = nullptr; // non-owning
    Shader*         m_wireShader  = nullptr; // non-owning
};
