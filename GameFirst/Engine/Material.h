//
// Created by JoshuaShin on 3/3/26.
//

#ifndef GAME_MATERIAL_H
#define GAME_MATERIAL_H
#include <SDL3/SDL.h>
#include <EngineMath.h>

#include "Renderer.h"

class AssetManager;
class Renderer;
class Shader;
class Texture;

struct alignas(16) MaterialConstantsData
{
    Vector3 c_diffuseColor; //12 bytes
    float _pad0; //4 bytes

    Vector3 c_specularColor; // 12bytes
    float c_specularPower; //4 bytes
};

class Material
{
public:
    Material() = default;
    ~Material() = default; //do nothing in destructor
    MaterialConstantsData& GetConstants() { return m_constants; } //getters for data
    const MaterialConstantsData& GetConstants() const { return m_constants; }
    void SetActive(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);
    void SetShader(Shader* shader) { m_shader = shader; }
    void SetTexture(int slot, const Texture* texture);
    void SetDiffuseColor (const Vector3& diffColor) { m_constants.c_diffuseColor = diffColor; }
    void SetSpecularColor(const Vector3& specColor) { m_constants.c_specularColor = specColor; }
    void SetSpecularPower(float power) { m_constants.c_specularPower = power; }
    static Material* StaticLoad(const char* fileName, AssetManager* pManager);
    bool Load(const char* fileName, AssetManager* pAssetManager);

private:
    MaterialConstantsData m_constants{};
    Shader* m_shader = nullptr;
    std::array<const Texture*, Renderer::TEXTURE_SLOT_TOTAL> m_textures{};
};


#endif //GAME_MATERIAL_H