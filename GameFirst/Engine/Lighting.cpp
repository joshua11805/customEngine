//
// Created by JoshuaShin on 3/3/26.
//

#include "Lighting.h"
#include "Camera.h"
#include "Renderer.h"

PointLightData* Lighting::AllocateLight()
{
    //loop through array of PL and find one with enabled == false, set to true, return by address
    for(int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        if (m_lightingConstants.c_pointLight[i].isEnabled == false)
        {
            m_lightingConstants.c_pointLight[i].isEnabled = true;
            return &m_lightingConstants.c_pointLight[i]; //return by address
        }
    }
    return nullptr;
}

void Lighting::SetAmbientLight(const Vector3* color)
{
    m_lightingConstants.c_ambient = *color;
}

const Vector3& Lighting::GetAmbientLight() const
{
    return m_lightingConstants.c_ambient;
}

void Lighting::SetActive(Camera* camera, SDL_GPUCommandBuffer* commandBuffer)
{
    Matrix4 worldToCamera = camera->returnWorldToCamera();
    Matrix4 viewToWorld = worldToCamera;
    viewToWorld.Invert();

    Vector3 cameraPos = viewToWorld.GetTranslation();
    m_lightingConstants.c_cameraPosition = cameraPos;

    SDL_PushGPUFragmentUniformData(
        commandBuffer,
        static_cast<Uint32>(Renderer::CONSTANT_FRAGMENT_LIGHTS),
        &m_lightingConstants,
        static_cast<Uint32>(sizeof(m_lightingConstants))
        );
}



