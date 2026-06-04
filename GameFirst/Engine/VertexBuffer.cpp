#include "VertexBuffer.h"

#include "Renderer.h"

VertexBuffer::VertexBuffer(Renderer* renderer,void* vertexData, uint32_t vertexDataSize, void* indexData, uint32_t numIndex, uint32_t indexStride)
{
    //intialize members
    m_renderer = renderer;
    m_numIndex = numIndex;
    m_vertexSize = vertexDataSize;
    if (indexStride == 2) { m_indexSize = SDL_GPU_INDEXELEMENTSIZE_16BIT; }
    else if (indexStride == 4){ m_indexSize = SDL_GPU_INDEXELEMENTSIZE_32BIT; }

    //create the vertex buffer
    m_vertexBuffer = m_renderer->CreateBuffer(vertexData, vertexDataSize, SDL_GPU_BUFFERUSAGE_VERTEX);
    //create the index buffer
    m_indexBuffer = m_renderer->CreateBuffer(indexData, numIndex * indexStride, SDL_GPU_BUFFERUSAGE_INDEX);
}

VertexBuffer::~VertexBuffer()
{
    SDL_ReleaseGPUBuffer(m_renderer->GetDevice(), m_vertexBuffer);
    SDL_ReleaseGPUBuffer(m_renderer->GetDevice(), m_indexBuffer);
}

void VertexBuffer::Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass) const
{
    //bind vertex buffer
    const SDL_GPUBufferBinding vertexBinding[] = {{ m_vertexBuffer, 0 }};
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBinding, 1);
    //bind index buffer
    const SDL_GPUBufferBinding indexBinding = { m_indexBuffer, 0};
    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, m_indexSize);


    SDL_DrawGPUIndexedPrimitives(renderPass, m_numIndex, 1, 0, 0, 0);

}
