#include "Actor.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Material.h"
#include "Shader.h"
#include"Component.h"

Actor::Actor(Mesh* mesh)
{
    m_mesh = mesh;
    m_POC.c_modelToWorld = Matrix4::Identity;
}

void Actor::RebuildMatrix()
{
    Matrix4 s   = Matrix4::CreateScale(m_scale);
    Matrix4 r   = Matrix4::CreateRotationZ(Math::ToRadians(m_eulerDeg.z))
                * Matrix4::CreateRotationX(Math::ToRadians(m_eulerDeg.x))
                * Matrix4::CreateRotationY(Math::ToRadians(m_eulerDeg.y));
    Matrix4 t   = Matrix4::CreateTranslation(m_position);
    m_POC.c_modelToWorld = s * r * t;
}

void Actor::SetPosition(Vector3 pos)
{
    m_position = pos;
    RebuildMatrix();
}

void Actor::SetEulerDeg(Vector3 deg)
{
    m_eulerDeg = deg;
    RebuildMatrix();
}

void Actor::SetScale(Vector3 scale)
{
    m_scale = scale;
    RebuildMatrix();
}

void Actor::SyncDecomposed()
{
    m_position = m_POC.c_modelToWorld.GetTranslation();
    m_scale    = m_POC.c_modelToWorld.GetScale();
    m_eulerDeg = Vector3::Zero; // rotation not decomposed; inspector will show 0
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


void Actor::Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, Shader* shaderOverride)
{
    SDL_PushGPUVertexUniformData(commandBuffer, Renderer::ConstantBuffer_Vertex::CONSTANT_VERTEX_RENDEROBJ, &m_POC,sizeof(m_POC));
    if (m_mesh)
        m_mesh->Draw(commandBuffer, renderPass, shaderOverride);
}

