#include "pch.h"
#include "Components/FPSCamera.h"
#include "Actor.h"
#include "Camera.h"
#include "Game.h"
#include "JsonUtil.h"

FPSCamera::FPSCamera(Actor* owner, Game* game)
    : Component(owner)
    , m_game(game)
{
    // Derive initial position and yaw from the actor's world transform
    Matrix4 mat = m_actor->GetWorldMat().c_modelToWorld;
    m_pos = mat.GetTranslation();
    Vector3 fwd = mat.GetXAxis();
    m_yaw = atan2f(fwd.y, fwd.x);

    game->SetMouseCapture(true);
}

FPSCamera::~FPSCamera()
{
    m_game->SetMouseCapture(false);
}

void FPSCamera::LoadProperties(const rapidjson::Value& properties)
{
    Component::LoadProperties(properties);
    GetFloatFromJSON(properties, "sensitivity", m_sensitivity);
    GetFloatFromJSON(properties, "eyeHeight", m_eyeHeight);
    GetFloatFromJSON(properties, "moveSpeed", m_moveSpeed);
}

void FPSCamera::Update(float deltaTime)
{
    if (m_captured)
    {
        // Release the cursor on Escape
        if (m_game->IsKeyJustPressed(SDL_SCANCODE_ESCAPE))
        {
            m_captured = false;
            m_game->SetMouseCapture(false);
            return;
        }
    }
    else
    {
        // Cursor is free — re-capture when the user clicks back into the window.
        // Swallow this frame's input so the refocus click neither fires nor jerks the view.
        if (m_game->IsMouseButtonJustPressed(SDL_BUTTON_MASK(SDL_BUTTON_LEFT)))
        {
            m_captured = true;
            m_game->SetMouseCapture(true);
        }
        return;
    }

    // Mouse look: raw pixel delta scaled by sensitivity
    Vector2 mouse = m_game->GetRawRelativeMouse();
    m_yaw   += mouse.x * m_sensitivity;
    m_pitch += mouse.y * m_sensitivity; // mouse down = look down
    m_pitch  = Math::Clamp(m_pitch, s_pitchMin, s_pitchMax);

    // WASD ground-plane movement (yaw only — no pitch on movement direction)
    float fwdInput   = 0.0f;
    float rightInput = 0.0f;
    if (m_game->IsKeyHeld(SDL_SCANCODE_W) || m_game->IsKeyHeld(SDL_SCANCODE_UP))    fwdInput   += 1.0f;
    if (m_game->IsKeyHeld(SDL_SCANCODE_S) || m_game->IsKeyHeld(SDL_SCANCODE_DOWN))  fwdInput   -= 1.0f;
    if (m_game->IsKeyHeld(SDL_SCANCODE_D) || m_game->IsKeyHeld(SDL_SCANCODE_RIGHT)) rightInput += 1.0f;
    if (m_game->IsKeyHeld(SDL_SCANCODE_A) || m_game->IsKeyHeld(SDL_SCANCODE_LEFT))  rightInput -= 1.0f;

    // Derive forward/right from yaw rotation matrix (same convention as Player)
    Matrix4 yawMat = Matrix4::CreateRotationZ(m_yaw);
    Vector3 fwd   = yawMat.GetXAxis(); // local X = world forward
    Vector3 right = yawMat.GetYAxis(); // local Y = world right
    m_pos += (fwd * fwdInput + right * rightInput) * (m_moveSpeed * deltaTime);
    m_actor->SetWorldMat(Matrix4::CreateTranslation(m_pos));

    // Build camera matrix: tilt by pitch around local Y, spin by yaw around world Z
    Vector3 eyePos = m_pos + Vector3(0.0f, 0.0f, m_eyeHeight);
    Matrix4 cameraToWorld = Matrix4::CreateRotationY(m_pitch)
                          * Matrix4::CreateRotationZ(m_yaw)
                          * Matrix4::CreateTranslation(eyePos);
    cameraToWorld.Invert();
    m_game->GetCamera()->SetWorldToCamera(cameraToWorld);
}

Vector3 FPSCamera::GetForward() const
{
    Matrix4 rot = Matrix4::CreateRotationY(m_pitch) * Matrix4::CreateRotationZ(m_yaw);
    return rot.GetXAxis();
}
