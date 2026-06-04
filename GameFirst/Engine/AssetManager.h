#pragma once
#include "AssetCache.h"
#include "Shader.h"
#include "Texture.h"

class Material;
class Mesh;
class Skeleton;
class Animation;

class AssetManager
{
public:
    AssetManager();
    ~AssetManager();
    Shader* GetShader(const std::string& shaderName);
    void SetShader(const std::string& shaderName, Shader* pShader);
    Texture* LoadTexture(const std::string& fileName);
    Material* LoadMaterial(const std::string& materialName);
    // Register an externally-created asset so the cache owns/frees it (used by the glTF importer)
    void CacheTexture(const std::string& key, Texture* texture);
    void CacheMaterial(const std::string& key, Material* material);
    Mesh* LoadMesh(const std::string& fileName);
    Skeleton* LoadSkeleton(const std::string& fileName);
    Animation* LoadAnimation(const std::string& fileName);
    void Clear();

private:
    AssetCache<Shader> m_shaderCache;
    AssetCache<Texture> m_textureCache;
    AssetCache<Material> m_materialCache;
    AssetCache<Mesh> m_meshCache;
    AssetCache<Skeleton> m_skeletonCache;
    AssetCache<Animation> m_animationCache;
};
