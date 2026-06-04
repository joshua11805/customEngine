#pragma once
#include "Component.h"
#include "EngineMath.h"
#include <rapidjson/document.h>

class Game;

class FPSCamera : public Component
{
public:
    FPSCamera(Actor* owner, Game* game);
    ~FPSCamera();

    void Update(float deltaTime) override;
    void LoadProperties(const rapidjson::Value& properties) override;

    // Forward direction in world space — use for raycasting
    Vector3 GetForward() const;
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

private:
    Game* m_game = nullptr;

    bool m_captured = true; // true when the cursor is locked for mouse-look
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_sensitivity = 0.003f;
    float m_eyeHeight = 64.0f;
    float m_moveSpeed = 400.0f;
    static constexpr float s_pitchMin = -1.3962f; // -80 degrees
    static constexpr float s_pitchMax =  1.3962f; // +80 degrees

    Vector3 m_pos = Vector3::Zero;
};
