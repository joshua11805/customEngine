#include "Actor.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Material.h"
#include"Component.h"

Actor::Actor(Mesh* mesh)
{
    m_mesh = mesh;
    m_POC.c_modelToWorld = Matrix4::CreateRotationZ(Math::ToRadians(45.0f));
}
Actor::~Actor()
{
    for (Component* component : m_components)
    {
        delete component;
    }
    m_components.clear();
}

void Actor::Update(float deltaTime)
{
    for (Component* component : m_components)
    {
        component->Update(deltaTime);
    }
}


void Actor::Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
    SDL_PushGPUVertexUniformData(commandBuffer, Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_RENDEROBJ, &m_POC,sizeof(m_POC));
    if (m_mesh)
        m_mesh->Draw(commandBuffer, renderPass);
}

