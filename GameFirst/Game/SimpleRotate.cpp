#include "SimpleRotate.h"
#include "Actor.h"
#include "JsonUtil.h"

void SimpleRotate::LoadProperties(const rapidjson::Value& properties)
{
    GetFloatFromJSON(properties, "speed", m_speed);
}

void SimpleRotate::Update(float deltaTime)
{
    //rotate about the z-axis
    m_currAngle += m_speed * deltaTime;

    if (m_currAngle > Math::TwoPi)
        m_currAngle -= Math::TwoPi;

    //storing and spinning the original matrix.
    //adding rotation to the mat every frame made it the rotation stack exponentially (bad idea)
    if (!m_init)
    {
        m_originalMat = m_actor->GetTransform();
        m_init = true;
    }


    Matrix4 newMat = Matrix4::CreateRotationZ(m_currAngle) * m_originalMat;
    m_actor->SetTransform(newMat);
}

