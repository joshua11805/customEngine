#include "pch.h"
#include "Components/TargetMover.h"
#include "Actor.h"
#include "JsonUtil.h"

void TargetMover::LoadProperties(const rapidjson::Value& properties)
{
    Component::LoadProperties(properties);
    GetVectorFromJSON(properties, "axis", m_axis);
    m_axis.Normalize();
    GetFloatFromJSON(properties, "angularFreq", m_angularFreq);
    GetFloatFromJSON(properties, "amplitude", m_amplitude);
    float timeOffset = 0.0f;
    if (GetFloatFromJSON(properties, "timeOffset", timeOffset))
        m_time = timeOffset;
}

void TargetMover::Update(float deltaTime)
{
    if (!m_init)
    {
        Matrix4 mat = m_actor->GetTransform();
        m_origin = mat.GetTranslation();
        m_scale  = mat.GetXAxis().Length();
        m_init   = true;
    }

    m_time += deltaTime;
    float offset = sinf(m_time * m_angularFreq) * m_amplitude;
    Vector3 newPos = m_origin + m_axis * offset;

    m_actor->SetTransform(Matrix4::CreateScale(m_scale) * Matrix4::CreateTranslation(newPos));
}
