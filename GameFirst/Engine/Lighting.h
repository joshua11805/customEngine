//
// Created by JoshuaShin on 3/3/26.
//

#ifndef GAME_LIGHTING_H
#define GAME_LIGHTING_H
#include "EngineMath.h"

class Camera;

struct alignas(16) PointLightData
{
    Vector3 lightColor; //12 bytes;
    float lightPadding; // 4 bytes;

    Vector3 position;
    float posPadding;

    float innerRadius; // 4 bytes
    float outerRadius; //4 bytes
    bool isEnabled; // 1 byte;
    uint8_t _pad0;
    uint8_t _pad1;
    uint8_t _pad2;
    //careful isEnabled is defaulted to 0 here
};

class Lighting
{
public:
    static constexpr int MAX_POINT_LIGHTS = 8;
    struct alignas(16) LightingConstants
    {
        Vector3 c_cameraPosition;
        float camPosPadding;

        Vector3 c_ambient;
        float c_ambientPadding;

        PointLightData c_pointLight[MAX_POINT_LIGHTS];
    };
    Lighting() = default;
    ~Lighting() = default;
    PointLightData* AllocateLight();
    void FreeLight(PointLightData* pLight) { pLight->isEnabled = false; }
    void SetAmbientLight(const Vector3* color);
    const Vector3 &GetAmbientLight() const;
    void SetActive(Camera* camera, SDL_GPUCommandBuffer* commandBuffer);


private:
    LightingConstants m_lightingConstants;
};


#endif //GAME_LIGHTING_H