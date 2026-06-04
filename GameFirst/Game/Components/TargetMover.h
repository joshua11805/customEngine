#pragma once
#include "Component.h"
#include "EngineMath.h"
#include <rapidjson/document.h>

// Oscillates an actor sinusoidally along an axis.
// Used for moving aim-trainer targets.
class TargetMover : public Component
{
public:
    TargetMover(Actor* owner) : Component(owner) {}

    void LoadProperties(const rapidjson::Value& properties) override;
    void Update(float deltaTime) override;

private:
    Vector3 m_axis      = Vector3(0.0f, 1.0f, 0.0f);
    float   m_angularFreq = 2.0f;   // radians per second
    float   m_amplitude   = 150.0f; // max displacement in world units
    float   m_time        = 0.0f;

    // Cached from actor on first update
    bool    m_init   = false;
    Vector3 m_origin = Vector3::Zero;
    float   m_scale  = 1.0f;
};
