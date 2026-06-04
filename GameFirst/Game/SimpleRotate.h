#ifndef GAME_SIMPLEROTATE_H
#define GAME_SIMPLEROTATE_H
#include "../Engine/Component.h"
#include "../Engine/EngineMath.h"

class SimpleRotate : public Component
{
public:
    SimpleRotate(Actor* owner) : Component(owner) {}
    void LoadProperties(const rapidjson::Value& properties) override;
    void Update(float deltaTime) override;
private:
    float m_speed = 1.0f;
    float m_currAngle = 0.0f;
    bool m_init = false;
    Matrix4 m_originalMat;
};


#endif //GAME_SIMPLEROTATE_H