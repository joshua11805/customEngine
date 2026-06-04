//
// Created by JoshuaShin on 3/4/26.
//

#include "PointLight.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "Lighting.h"

PointLight::PointLight(Actor* actor, Lighting* lighting) : Component(actor), m_lighting(lighting)
{
    m_pointLight = lighting->AllocateLight();
}
PointLight::~PointLight()
{
    m_lighting->FreeLight(m_pointLight);
}

void PointLight::LoadProperties(const rapidjson::Value& properties)
{
    Vector3 lightColor;
    GetVectorFromJSON(properties, "lightColor", lightColor);
    m_pointLight->lightColor = lightColor;

    GetFloatFromJSON(properties, "innerRadius", m_pointLight->innerRadius);
    GetFloatFromJSON(properties, "outerRadius", m_pointLight->outerRadius);
}

void PointLight::Update(float deltaTime)
{
    m_pointLight->position = m_actor->GetTransformVec();
}
