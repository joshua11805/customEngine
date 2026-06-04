#pragma once
#include <SDL3/SDL.h>

class Texture;

class Renderer {
public:
    // use these to match the texture registers in your hlsl
    enum TextureSlot
    {
        TEXTURE_SLOT_DIFFUSE,
        TEXTURE_SLOT_TANGENT, //adding texture slot for tangent
        TEXTURE_SLOT_TOTAL      // this goes last - counts the number in the enum
    };

    // use these to bind your constant buffers to the vertex shaders in your hlsl
    enum ConstantBuffer_Vertex
    {
        CONSTANT_VERTEX_RENDEROBJ,
        CONSTANT_VERTEX_CAMERA,
        CONSTANT_VERTEX_SKINNING,
    };

    // use these to bind your constant buffers to the fragment shaders in your hlsl
    enum ConstantBuffer_Fragment
    {
        CONSTANT_FRAGMENT_MATERIAL,
        CONSTANT_FRAGMENT_LIGHTS,
    };

    Renderer();
    ~Renderer();
    static Renderer* Get() { return s_theRenderer; }

    bool Init(int width, int height);
    void Shutdown();
    int GetScreenWidth() const { return m_screenWidth; }
    int GetScreenHeight() const { return m_screenHeight; }
    SDL_Window* GetWindow() { return m_window; }
    SDL_GPUDevice* GetDevice() { return m_device; }
    SDL_GPUTexture* GetBackBuffer() { return m_backBuffer; }
    SDL_GPUSampler* GetSampler() { return m_sampler; }

    void OnResize(int w, int h);
    SDL_GPUBuffer* CreateBuffer(void* data, uint32_t dataSize, SDL_GPUBufferUsageFlags usage);
    void UploadBuffer(SDL_GPUBuffer* buffer, void* data, uint32_t dataSize);

    SDL_GPUCommandBuffer* BeginCommandBuffer();
    void EndCommandBuffer(SDL_GPUCommandBuffer* commandBuffer);

    SDL_GPURenderPass* ClearScreen(SDL_GPUCommandBuffer* commandBuffer, const SDL_FColor& color);
    SDL_GPURenderPass* ClearScreen(SDL_GPUCommandBuffer* commandBuffer, const SDL_FColor& color, Texture* renderTarget);
    SDL_GPURenderPass* BeginRenderPass(SDL_GPUCommandBuffer* commandBuffer, Texture* renderTarget = nullptr);
    void EndRenderPass(SDL_GPURenderPass* renderPass);
    SDL_GPUTextureFormat GetTextureFormat() { return m_textureFormat; }
    SDL_GPUTexture* GetTexture() { return m_depthTexture; }

private:
    static Renderer* s_theRenderer;
    SDL_GPUTexture* m_depthTexture;
    SDL_GPUTextureFormat m_textureFormat;
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;    // we just re-use this same sampler for everything
    SDL_GPUTexture* m_backBuffer = nullptr; // this only exists between BeginCommandBuffer() and EndCommandBuffer()
};

