//
// Created by JoshuaShin on 2/1/26.
//

#include "Camera.h"
#include "Renderer.h"

Camera::Camera(Renderer* renderer)
{
    m_worldToCamera = Matrix4::CreateTranslation(Vector3(500.0f, 0.0f, 0.0f));
    m_projectionMatrix = Matrix4::CreateRotationY(-Math::PiOver2)
    * Matrix4::CreateRotationZ(-Math::PiOver2)
    * Matrix4::CreatePerspectiveFOV(Math::ToRadians(70.0f), renderer->GetScreenWidth(), renderer->GetScreenHeight(), 25.0f, 100000.0f);
}

void Camera::SetScreenSize(int w, int h)
{
    m_projectionMatrix = Matrix4::CreateRotationY(-Math::PiOver2)
        * Matrix4::CreateRotationZ(-Math::PiOver2)
        * Matrix4::CreatePerspectiveFOV(Math::ToRadians(70.0f), w, h, 25.0f, 100000.0f);
}

void Camera::SetActive(SDL_GPUCommandBuffer* commandBuffer)
{
    m_PCC.c_viewProj = m_worldToCamera * m_projectionMatrix;
    SDL_PushGPUVertexUniformData(commandBuffer, Renderer::CONSTANT_VERTEX_CAMERA, &m_PCC, sizeof(m_PCC));
}

