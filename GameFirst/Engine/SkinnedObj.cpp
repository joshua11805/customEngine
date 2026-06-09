//
// Created by JoshuaShin on 3/19/26.
//
#include "pch.h"
#include "SkinnedObj.h"
#include "Mesh.h"
#include "Actor.h"
#include "Shader.h"

SkinnedObj::SkinnedObj(Mesh* mesh) : Actor(mesh)
{
    //fill in the skin matrix so mesh renders in bind pose at first
    for (int i = 0; i < MAX_SKELETON_BONES; ++i)
    {
        m_skinConstants.c_skinMatrix[i] = Matrix4::Identity;
    }

}

Shader* SkinnedObj::PickSceneShader(int mode)
{
    // SceneViewMode values (mirrored here to avoid EditorLayer.h dependency):
    // 0=Lit, 1=Unlit, 2=Wireframe, 3=WireframeOnShaded, 4=LightContrib, 5=VertexColor
    switch (mode)
    {
        case 2: return m_sceneWire;   // Wireframe
        default: return m_sceneLit;   // Lit, Unlit, WireframeOnShaded (lit sub-pass), LightContrib
    }
}

void SkinnedObj::Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, Shader* shaderOverride)
{
    SDL_PushGPUVertexUniformData(
        commandBuffer,
        Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_SKINNING,
        &m_skinConstants,
        sizeof(m_skinConstants)
    );

    SDL_PushGPUVertexUniformData(commandBuffer, Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_RENDEROBJ, &m_POC, sizeof(m_POC));
    if (m_mesh)
        m_mesh->Draw(commandBuffer, renderPass, shaderOverride);
}
