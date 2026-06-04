#pragma once
#include <vector>

class AssetManager;
class Material;
class Renderer;
class VertexBuffer;

class Mesh {
public:
    Mesh(VertexBuffer* vertexBuffer, Material* material);
    ~Mesh();

    bool Load(void* vertexData, uint32_t vertexDataSize, void* indexData, uint32_t numIndex, uint32_t indexStride, Material* material);
    bool Load(const char* fileName, AssetManager* pAssetManager);
    static Mesh* StaticLoad(const char* fileName, AssetManager* pAssetManager);
    bool IsSkinned() const { return m_isSkinned; }
    void Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);
    void AddSubMesh(Mesh* sub); // takes ownership; used by multi-material importers

protected:
    bool m_isSkinned = false;
    VertexBuffer* m_vertexBuffer = nullptr;
    Material* m_material = nullptr;
    std::vector<Mesh*> m_subMeshes;
};
