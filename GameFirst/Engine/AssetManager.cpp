#include "pch.h"
#include "AssetManager.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "Animation.h"



AssetManager::AssetManager() : m_shaderCache(this), m_textureCache(this), m_materialCache(this), m_meshCache(this),
                               m_skeletonCache(this), m_animationCache(this)
{
}

AssetManager::~AssetManager()
{
}

void AssetManager::Clear()
{
    m_shaderCache.Clear();
    m_textureCache.Clear();
    m_materialCache.Clear();
    m_meshCache.Clear();
}

Shader* AssetManager::GetShader(const std::string& shaderName)
{
    return m_shaderCache.Get(shaderName);
}

void AssetManager::SetShader(const std::string& shaderName, Shader* pShader)
{
    m_shaderCache.Cache(shaderName, pShader);
}

Texture* AssetManager::LoadTexture(const std::string& fileName)
{
    return m_textureCache.Load(fileName);
}

Material* AssetManager::LoadMaterial(const std::string& materialName)
{
    return m_materialCache.Load(materialName);
}

void AssetManager::CacheTexture(const std::string& key, Texture* texture)
{
    m_textureCache.Cache(key, texture);
}

void AssetManager::CacheMaterial(const std::string& key, Material* material)
{
    m_materialCache.Cache(key, material);
}

Mesh* AssetManager::LoadMesh(const std::string& fileName)
{
    return m_meshCache.Load(fileName);
}

Skeleton* AssetManager::LoadSkeleton(const std::string& fileName)
{
    return m_skeletonCache.Load(fileName);
}

Animation* AssetManager::LoadAnimation(const std::string& fileName)
{
    return m_animationCache.Load(fileName);
}

