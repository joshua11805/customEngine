#if 1
#include "pch.h"
#include "Actor.h"
#include "Camera.h"
#include "Components/FollowCam.h"
#include "Game.h"
#include "JsonUtil.h"


FollowCam::FollowCam(Actor* owner, Game* pGame)
    : Component(owner)
    , m_game(pGame)
{
	Matrix4 mat = m_actor->GetWorldMat().c_modelToWorld;
    //90 degree rotation for orientation of model?
	mat = Matrix4::CreateRotationX(Math::PiOver2) * mat;
}

void FollowCam::LoadProperties(const rapidjson::Value& properties)
{
    Component::LoadProperties(properties);
    GetVectorFromJSON(properties, "offset", m_offset);
    GetVectorFromJSON(properties, "center", mCenter);
    GetFloatFromJSON(properties, "yawSpeed", m_yawSpeed);
    GetFloatFromJSON(properties, "pitchSpeed", m_pitchSpeed);
    if (GetFloatFromJSON(properties, "minElevation", m_elvMin))
        m_elvMin = Math::ToRadians(m_elvMin);
    if (GetFloatFromJSON(properties, "maxElevation", m_elvMax))
        m_elvMax = Math::ToRadians(m_elvMax);
    m_radius = m_offset.Length();
    Vector3 offsetXY = m_offset;
    offsetXY.z = 0.0f;
    float dxy = offsetXY.Length();
    m_azimuth = atan2f(-m_offset.y, -m_offset.x);
    m_elevation = atan2f(m_offset.z, dxy);
}

void FollowCam::Update(float deltaTime)
{
    if (m_game->GetMouseButtons() & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))
    {   // mouse control to move the camera only when the left-button is held
        m_azimuth += m_game->GetRelativeMouse().x * m_yawSpeed;
        m_elevation += m_game->GetRelativeMouse().y * m_pitchSpeed;
        m_elevation = Math::Clamp(m_elevation, m_elvMin, m_elvMax);
    }

    // recalculate the location of the camera relative to the target (Player)
    Vector3 p;
    p.z = m_radius * sinf(m_elevation);
    float dxy = m_radius * cosf(m_elevation);
    p.x = -dxy * cosf(m_azimuth);
    p.y = -dxy * sinf(m_azimuth);

    // copy the camera's position into the camera matrix
    Camera* pCamera = m_game->GetCamera();
    Matrix4 mat = Matrix4::CreateRotationY(m_elevation)
        * Matrix4::CreateRotationZ(m_azimuth)
        * Matrix4::CreateTranslation(m_actor->GetWorldMat().c_modelToWorld.GetTranslation() + mCenter + p);
    mat.Invert();
    pCamera->SetWorldToCamera(mat);
}
#endif