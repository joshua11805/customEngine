#if 1
#pragma once
#include "../../Engine/Character.h"
#include "EngineMath.h"

class Game;

class Player : public Character
{
public:
    Player(SkinnedObj* skinnedObj, Game* pGame, AssetManager* assetManager);

    void Update(float deltaTime) override;

protected:
    enum class State
    {
        UNKNOWN,
        IDLE,
        FALL,
        WALK,
        JUMP,
        LAND,
    };

    bool CheckGround(float* groundHeight);
    void SetState(State newState);
    void ChangeState();
    void UpdateState(float deltaTime);

    Game* m_game = nullptr;
    State m_state = State::UNKNOWN;
	float m_moveSpeed = 0.0f;
	float m_heading = 0.0f;
    Vector3 m_pos = Vector3::Zero;
    Vector3 m_vel = Vector3::Zero;
    float m_prevAnimTime = 0.0f;
};
#endif