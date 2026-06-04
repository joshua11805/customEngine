//
// Created by JoshuaShin on 3/3/26.
//

#include "Material.h"

#include "AssetManager.h"
#include "JsonUtil.h"
#include "Shader.h"
#include "Texture.h"

void Material::SetTexture(int slot, const Texture* texture)
{
    if (slot >= 0 && slot < static_cast<int>(m_textures.size()))
    {
        m_textures[slot] = texture;
    }
}

bool Material::Load(const char* fileName, AssetManager* pAssetManager)
{
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        return false;
    }
    std::stringstream fileStream;
    fileStream << file.rdbuf();
    std::string contents = fileStream.str();
    rapidjson::StringStream jsonStr(contents.c_str());
    rapidjson::Document doc;
    doc.ParseStream(jsonStr);
    if (!doc.IsObject())
    {
        return false;
    }
    std::string str = doc["metadata"]["type"].GetString();
    int ver = doc["metadata"]["version"].GetInt();
    // Check the metadata
    if (!doc["metadata"].IsObject() ||
    str != "itpmat" ||
    ver != 1)
    {
        return false;
    }
    // Load Shader
    std::string shaderName;
    if (false == GetStringFromJSON(doc, "shader", shaderName))
        return false;
    m_shader = pAssetManager->GetShader(shaderName);

    DbgAssert(nullptr != m_shader, "Material unable to load shader");
    { // Load textures
        const rapidjson::Value& textures = doc["textures"];
        if (textures.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < textures.Size(); i++)
            {
                if (i < Renderer::TEXTURE_SLOT_TOTAL)
                {
                    m_textures[i] = pAssetManager->LoadTexture(textures[i].GetString());
                }
            }
        }
    }
    // Load the Lighting Parameters
    GetVectorFromJSON(doc, "diffuseColor", m_constants.c_diffuseColor);
    GetVectorFromJSON(doc, "specularColor", m_constants.c_specularColor);
    GetFloatFromJSON(doc, "specularPower", m_constants.c_specularPower);
    return true;
}

void Material::SetActive(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
    //first set shader active
    if (m_shader)
    {
        m_shader->SetActive(renderPass);
    }
    //then loop through and set all textures active on matching slot
    for (int slot = 0; slot < Renderer::TEXTURE_SLOT_TOTAL; ++slot)
    {
        const Texture* tex = m_textures[slot];
        if (!tex) //watch out for null textures
        {
            continue;
        }
        else
        {
            tex->SetActive(renderPass, slot);
        }
    }
    //push constants to specified slot
    SDL_PushGPUFragmentUniformData(
        commandBuffer,
        static_cast<Uint32>(Renderer::CONSTANT_FRAGMENT_MATERIAL),
        (void*)&m_constants,
        static_cast<Uint32>(sizeof(m_constants)));
}

Material* Material::StaticLoad(const char* fileName, AssetManager* pManager)
{
    Material* mat = new Material();
    if (mat->Load(fileName, pManager) == false)
    {
        return mat;
    }
    return mat;
}

