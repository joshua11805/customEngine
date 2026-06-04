#pragma once
#include <SDL3/SDL.h>

class Renderer;
class Texture;

// Shader encapsulates the Vertex Shader, the Fragment Shader, and the entire GPU Pipeline
// That means a Shader is set up to handle only 1 format of texture output, and a unique set of blend options
class Shader
{
public:
    Shader(Renderer* renderer, const char* filename);
    ~Shader();
    void SetColorTarget(Texture* colorTarget=nullptr);
    bool CreatePipeline(const SDL_GPUVertexAttribute* vertexAttributes, uint32_t numVertexAttributes, uint32_t vertexStride);
    void SetActive(SDL_GPURenderPass* renderPass);
    void SetZWrite(bool zWrite) { m_zWrite = zWrite; }
    void SetZTest(bool zTest) { m_zTest = zTest; }
    void SetBlend(bool enable, SDL_GPUBlendFactor sourceFactor, SDL_GPUBlendFactor distanceFactor);

private:
    Renderer* m_renderer = nullptr;
    // we're only supporting a single color target
    Texture* m_colorTarget = nullptr;
    // shaders
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;
    // the shaders are combined into a pipeline
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;

    bool Load(const char* filename);
    bool m_zWrite = true;
    bool m_zTest = true;
    bool m_blendEnable = false;
    SDL_GPUBlendFactor m_srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
    SDL_GPUBlendFactor m_dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;

};