#pragma once
#include "Actor.h"
#include "VertexFormat.h"
#include <vector>

class VertexBuffer;
class Shader;

// One NxN quad patch of procedural terrain.
// Generated on CPU, uploaded to a VertexBuffer, drawn as a normal Actor.
class TerrainChunk : public Actor
{
public:
    struct GenParams
    {
        int   resolution   = 64;    // verts per side (quads = res-1)
        float tileSize     = 200.f; // world units per chunk side
        float heightScale  = 500.f; // max vertex Z displacement
        float noiseScale   = 0.005f;// noise frequency (higher = more varied)
        int   octaves      = 6;
        float persistence  = 0.5f;
        float lacunarity   = 2.0f;
        int   seed         = 42;
    };

    // chunkX/chunkZ: integer grid coordinates (chunk origin = chunkX*tileSize, chunkZ*tileSize)
    TerrainChunk(int chunkX, int chunkZ, const GenParams& params);
    ~TerrainChunk() override;

    // Draws the terrain chunk. Uses shaderOverride when provided (e.g. wireframe scene modes),
    // otherwise selects m_terrainShader or m_terrainShaderGame based on s_gamePass.
    void Draw(SDL_GPUCommandBuffer* commandBuffer,
              SDL_GPURenderPass*   renderPass,
              Shader*              shaderOverride = nullptr) override;

    // Set terrain shaders — scene (editor), game pass, and scene wireframe. Non-owning.
    void SetTerrainShaders(Shader* scene, Shader* game, Shader* sceneWire = nullptr)
        { m_terrainShader = scene; m_terrainShaderGame = game; m_terrainShaderWire = sceneWire; }

    // Call before drawing the game pass so chunks switch to the game shader.
    static void SetGamePass(bool isGame) { s_gamePass = isGame; }
    static bool IsGamePass() { return s_gamePass; }

    // Returns a scene-view override shader when the mode needs a different pipeline
    // (e.g. wireframe). Returns nullptr to use the default internal shader.
    Shader* PickSceneShader(int mode) override;

    int GetChunkX() const { return m_chunkX; }
    int GetChunkZ() const { return m_chunkZ; }

private:
    int m_chunkX = 0;
    int m_chunkZ = 0;

    // Owned GPU resources (not via AssetManager — terrain owns them directly)
    VertexBuffer* m_terrainVB          = nullptr;
    Shader*       m_terrainShader      = nullptr; // non-owning; scene Lit pass
    Shader*       m_terrainShaderGame  = nullptr; // non-owning; game pass
    Shader*       m_terrainShaderWire  = nullptr; // non-owning; scene Wireframe pass

    static bool s_gamePass; // set by render loop before each pass

    void Generate(const GenParams& params);

    // Map a height [0,1] to a Color4 band
    static Color4 HeightColor(float t);
};
