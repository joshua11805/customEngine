//
// Created by JoshuaShin on 2/1/26.
//

#ifndef GAME_VERTEXBUFFER_H
#define GAME_VERTEXBUFFER_H

struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
class Renderer;

class VertexBuffer
{
public:
    VertexBuffer(Renderer* renderer, void* vertexData, uint32_t vertexDataSize, void* indexData, uint32_t numIndex, uint32_t indexStride);
    ~VertexBuffer();
    void Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass) const;
    //uint32_t GetSize() { return m_numVertices * m_vertexStride; }
    //uint32_t GetNumVertices() { return m_numVertices; }
    //uint32_t GetVertexStride() { return m_vertexStride; }

private:
    Renderer* m_renderer = nullptr;
    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer = nullptr;
    SDL_GPUIndexElementSize m_indexSize = SDL_GPU_INDEXELEMENTSIZE_16BIT;
    uint32_t m_numIndex;
    uint32_t m_numVertices;
    uint32_t m_vertexSize;
};


#endif //GAME_VERTEXBUFFER_H