#pragma once

class AssetManager;

// This class encapsulates Textures for use as both resources and render targets
class Texture {
public:
    Texture();
    ~Texture();

    void Free();
    bool Load(const char* fileName);
    // Decode an in-memory image (PNG/JPG/etc), e.g. an image embedded in a GLB
    bool LoadFromMemory(const unsigned char* data, int len, const char* name);
    // Create a solid 1x1 RGBA texture (used as a fallback when a material has no texture)
    bool CreateSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const char* name);
    static Texture* StaticLoad(const char* fileName, AssetManager* pManager);
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    SDL_GPUTextureFormat GetFormat() const { return m_format; }
    SDL_GPUTexture* GetTexture() { return m_texture; }
    void SetActive(SDL_GPURenderPass* renderPass, int slot) const;
    // Create from an 8-bit alpha-only bitmap (e.g. a stb_truetype glyph atlas).
    // Stored internally as RGBA (255,255,255,alpha) so the shader can tint it.
    bool CreateFromAlpha(const uint8_t* alphaData, int width, int height, const char* name);
    bool CreateRenderTarget(int width, int height, SDL_GPUTextureFormat format, const char* name);
    void Resize(int newWidth, int newHeight);

private:
    int m_width = 0;
    int m_height = 0;
    SDL_GPUTextureFormat m_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    SDL_GPUTexture* m_texture = nullptr;
    std::string m_name;

    // Shared GPU upload path: creates an RGBA8 sampler texture from raw 4-channel pixels
    bool UploadRGBA(const unsigned char* pixels, int width, int height, const char* name);
};
