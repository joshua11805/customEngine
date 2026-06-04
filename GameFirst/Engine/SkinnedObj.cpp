//
// Created by JoshuaShin on 3/19/26.
//
#include "SkinnedObj.h"
#include "Mesh.h"
#include "Actor.h"

SkinnedObj::SkinnedObj(Mesh* mesh) : Actor(mesh)
{
    //fill in the skin matrix so mesh renders in bind pose at first
    for (int i = 0; i < MAX_SKELETON_BONES; ++i)
    {
        m_skinConstants.c_skinMatrix[i] = Matrix4::Identity;
    }

}

void SkinnedObj::Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
    // upload the skin matrices to vertex shader slot 2
    SDL_PushGPUVertexUniformData(
        commandBuffer,
        Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_SKINNING,
        &m_skinConstants,
        sizeof(m_skinConstants)
    );

    //not sure if we're supposed to do Actor::Draw(), so made m_POC protected and copying code from the func
    // per-object constants same as actor
    SDL_PushGPUVertexUniformData(commandBuffer, Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_RENDEROBJ, &m_POC,sizeof(m_POC));
    if (m_mesh)
        m_mesh->Draw(commandBuffer, renderPass);

}
