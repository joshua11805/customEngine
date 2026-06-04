#include "pch.h"
#include "Texture.h"
#include "Profiler.h"
#include "Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture::Texture()
{
}

Texture::~Texture()
{
    Free();
}

/// Release all resources used by the Texture and reset it back to an empty status
/// This function is automatically called by the destructor
/// You can also call this function manually if you want to change the usage of the Texture
void Texture::Free()
{
    if (nullptr != m_texture)
    {
        auto device = Renderer::Get()->GetDevice();
        SDL_ReleaseGPUTexture(device, m_texture);
        m_texture = nullptr;
    }
    m_width = 0;
    m_height = 0;
}

/// Load the texture from a file
/// At this point, only png files are supported
/// At this point, only 4-channel (RGBA) textures are supported
/// @param fileName the name of the texture file to load ("Assets/Textures/Cube.png")
/// @return true on success or false on failure
bool Texture::Load(const char* fileName)
{
    PROFILE_SCOPE(TextureLoad);

    int channels = 0;

    // attempt to load the file, forcing 4-channel (RGBA) output
    unsigned char* image = stbi_load(fileName, &m_width, &m_height, &channels, 4);
    if (image == nullptr)
    {
        SDL_Log("Failed to load image %s: %s", fileName, stbi_failure_reason());
        return false;
    }

    bool ok = UploadRGBA(image, m_width, m_height, fileName);
    stbi_image_free(image);
    return ok;
}

/// Decode an image already resident in memory (e.g. an image chunk embedded in a GLB)
bool Texture::LoadFromMemory(const unsigned char* data, int len, const char* name)
{
    PROFILE_SCOPE(TextureLoad);

    int channels = 0;
    unsigned char* image = stbi_load_from_memory(data, len, &m_width, &m_height, &channels, 4);
    if (image == nullptr)
    {
        SDL_Log("Failed to decode embedded image %s: %s", name ? name : "(unnamed)", stbi_failure_reason());
        return false;
    }

    bool ok = UploadRGBA(image, m_width, m_height, name);
    stbi_image_free(image);
    return ok;
}

/// Create a solid 1x1 RGBA texture, used as a fallback diffuse map for materials with no texture
bool Texture::CreateSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const char* name)
{
    const unsigned char pixel[4] = { r, g, b, a };
    return UploadRGBA(pixel, 1, 1, name);
}

/// Shared GPU upload: creates an RGBA8 sampler texture from raw 4-channel pixel data
bool Texture::UploadRGBA(const unsigned char* pixels, int width, int height, const char* name)
{
    m_width = width;
    m_height = height;
    m_name = name ? name : "Texture";

    // Create the GPU texture
    auto device = Renderer::Get()->GetDevice();
    SDL_GPUTextureCreateInfo texInfo = { };
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = m_width;
    texInfo.height = m_height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    m_texture = SDL_CreateGPUTexture(device, &texInfo);
    if (m_texture == nullptr)
    {
        SDL_Log("Failed to create GPU texture: %s", m_name.c_str());
        return false;
    }
    SDL_SetGPUTextureName(device, m_texture, m_name.c_str());

    // Upload the texture to the GPU
    SDL_GPUTransferBufferCreateInfo createInfo = { };
    createInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    createInfo.size = m_width * m_height * 4;
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(device, &createInfo);

    void* textureTransferPtr = SDL_MapGPUTransferBuffer(device, textureTransferBuffer,false);
    SDL_memcpy(textureTransferPtr, pixels, m_width * m_height * 4);
    SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

    SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

    SDL_GPUTextureTransferInfo transferInfo = { };
    transferInfo.transfer_buffer = textureTransferBuffer;
    SDL_GPUTextureRegion texRegion = { };
    texRegion.texture = m_texture;
    texRegion.w = m_width;
    texRegion.h = m_height;
    texRegion.d = 1;
    SDL_UploadToGPUTexture(copyPass, &transferInfo, &texRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

    return true;
}

/*static*/ Texture* Texture::StaticLoad(const char* fileName, AssetManager* pManager)
{
    Texture* pTex = new Texture();
    if (false == pTex->Load(fileName))
    {
        delete pTex;
        return nullptr;
    }
    return pTex;
}

/// Set this texture to be active on the GPU on the specified texture slot
/// AKA bind this texture to a texture register
/// @param renderPass
/// @param slot which slot this texture is on (Renderer::TEXTURE_SLOT_DIFFUSE)
void Texture::SetActive(SDL_GPURenderPass* renderPass, int slot) const
{
    SDL_GPUTextureSamplerBinding binding{};
    binding.texture = m_texture;
    binding.sampler = Renderer::Get()->GetSampler();

    SDL_BindGPUFragmentSamplers(renderPass, slot, &binding, 1);

}

void Texture::Resize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0)
        return;
    SDL_GPUTextureFormat fmt = m_format; // Free() preserves m_format
    std::string name = m_name;           // preserve the debug name across the recreate
    Free();
    CreateRenderTarget(newWidth, newHeight, fmt, name.c_str());
}

bool Texture::CreateRenderTarget(int width, int height, SDL_GPUTextureFormat format, const char* name)
{
    Free();

    m_width = width;
    m_height = height;
    m_format = format;
    m_name = name ? name : "RenderTarget";

    auto device = Renderer::Get()->GetDevice();

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type                = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format              = format;
    texInfo.width               = width;
    texInfo.height              = height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels          = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    m_texture = SDL_CreateGPUTexture(device, &texInfo);
    if (m_texture == nullptr)
    {
        SDL_Log("Failed to create render target: %s", m_name.c_str());
        return false;
    }

    SDL_SetGPUTextureName(device, m_texture, m_name.c_str());
    return true;
}


