#if 1
#include "pch.h"
#include "Components/Player.h"

#include "AssetManager.h"
#include "Camera.h"
#include "Game.h"
#include "Physics.h"
#include "SkinnedObj.h"


static const float s_accel = 900.0f;
static const float s_moveSpeed = 300.0f;
static const float s_turnSpeed = Math::ToRadians(720.0f);
static const float s_gravity = 500.0f;
static const float s_jumpStart = 300.0f;


Player::Player(SkinnedObj* skinnedObj, Game* pGame, AssetManager* assetManager)
    : Character(static_cast<Actor*>(skinnedObj), assetManager)
    , m_game(pGame)
{
	Matrix4 mat = m_actor->GetWorldMat().c_modelToWorld;
    m_pos = mat.GetTranslation();
    Vector3 fwd = mat.GetXAxis();
	m_heading = atan2f(fwd.y, fwd.x);
}

void Player::Update(float deltaTime)
{
    // Read keyboard input
	Vector3 move = Vector3::Zero;
	if (m_game->IsKeyHeld(SDL_SCANCODE_UP) || m_game->IsKeyHeld(SDL_SCANCODE_W))
		move.x += 1.0f;
	if (m_game->IsKeyHeld(SDL_SCANCODE_DOWN) || m_game->IsKeyHeld(SDL_SCANCODE_S))
		move.x -= 1.0f;
	if (m_game->IsKeyHeld(SDL_SCANCODE_RIGHT) || m_game->IsKeyHeld(SDL_SCANCODE_D))
		move.y += 1.0f;
	if (m_game->IsKeyHeld(SDL_SCANCODE_LEFT) || m_game->IsKeyHeld(SDL_SCANCODE_A))
		move.y -= 1.0f;

    // camera relative controls
    Camera* pCamera = m_game->GetCamera();
    Matrix4 camMat = pCamera->returnWorldToCamera();
    camMat.Invert();
    Vector3 fwd = camMat.GetXAxis();
    fwd.z = 0.0f;
    fwd.Normalize();
    Vector3 right = camMat.GetYAxis();
    right.z = 0.0f;
    right.Normalize();
    move = move.x * fwd + move.y * right;

	float throttle = move.Length();
	if (throttle > 1.0f)
		throttle = 1.0f;

	if (throttle > 0.0f)
	{	// turn to heading
		float heading = atan2f(move.y, move.x);
		float angDelta = heading - m_heading;
		if (angDelta > Math::Pi)		//mrwTODO wrap angle
			angDelta -= Math::TwoPi;
		if (angDelta < -Math::Pi)
			angDelta += Math::TwoPi;
		float angSpd = s_turnSpeed * deltaTime;
		angDelta = Math::Clamp(angDelta, -angSpd, angSpd);
		m_heading += angDelta;
		if (m_heading > Math::Pi)		//mrwTODO wrap angle
			m_heading -= Math::TwoPi;
		if (m_heading < -Math::Pi)
			m_heading += Math::TwoPi;
	}

	{	// accelerate
		float speed = throttle * s_moveSpeed;
		float spdDelta = speed - m_moveSpeed;
		float accel = s_accel * deltaTime;
		spdDelta = Math::Clamp(spdDelta, -accel, accel);
		m_moveSpeed += spdDelta;
	}

    m_prevAnimTime = m_animTime;
    Character::Update(deltaTime);

    ChangeState();
    UpdateState(deltaTime);
}

/// Check for ground beneath the player's feet
/// @param groundHeight OUTPUT - If the player is over solid ground, the height of the ground will be copied here
/// @return true if the player is on the ground or false if not
bool Player::CheckGround(float* groundHeight)
{
    Vector3 start = m_pos + Vector3(0.0f, 0.0f, 10.0f);
    Vector3 end   = m_pos - Vector3(0.0f, 0.0f, 10.0f);

    Physics::LineSegment segment(start, end);
    Vector3 hitPoint;

    if (Physics::Get()->SegmentCast(segment, &hitPoint))
    {
        *groundHeight = hitPoint.z;
        return true;
    }

    return false;
}

/// Set the state of the player to newState and start any animations that go with that
/// @param newState
void Player::SetState(State newState)
{
    switch (newState)
    {
    case State::IDLE:
        SetAnim("idle");
        m_vel.z = 0.0f;
        break;
    case State::FALL:
        SetAnim("fall");
        break;
    case State::WALK:
        SetAnim("run");
        m_vel.z = 0.0f;
        break;
    case State::JUMP:
        SetAnim("jumpStart");
        m_vel.z = s_jumpStart;
        break;
    case State::LAND:
        SetAnim("land");
        m_vel.z = 0.0f;
        break;
    default:
        break;
    }
    m_state = newState;
}

/// Check to see if the player needs to switch states and switch as necessary
void Player::ChangeState()
{
    float groundHeight = 0.0f;
    bool isOverGround = CheckGround(&groundHeight);
    bool isOnGround = isOverGround && m_pos.z <= groundHeight;

    switch (m_state)
    {
    case State::UNKNOWN:
        SetState(State::IDLE);
        break;
    case State::LAND:
        if (IsAnimDone())
            SetState(State::IDLE);
        // no break... fall through
    case State::IDLE:
        if (m_game->IsKeyHeld(SDL_SCANCODE_SPACE))
            SetState(State::JUMP);
        else if (false == isOnGround)
            SetState(State::FALL);
        else if (m_vel.LengthSq() > 0.0001f)
            SetState(State::WALK);
        break;
    case State::WALK:
        if (m_game->IsKeyHeld(SDL_SCANCODE_SPACE))
            SetState(State::JUMP);
        else if (false == isOnGround)
            SetState(State::FALL);
        else if (m_vel.LengthSq() < 0.0001f)
            SetState(State::IDLE);
        break;
    case State::JUMP:
        if (IsAnimDone())
            SetAnim("jump");
        // no break... fall through
    case State::FALL:
        if (m_vel.z < 0.0f && isOnGround)
        {
            m_pos.z = groundHeight;
            SetState(State::LAND);
        }
        break;
    }
}

/// Update the player's current state
void Player::UpdateState(float deltaTime)
{
    Matrix4 mat = Matrix4::CreateRotationZ(m_heading);
    Vector3 vel = m_moveSpeed * mat.GetXAxis();
    m_vel.x = vel.x;
    m_vel.y = vel.y;

    switch (m_state)
    {
    case State::IDLE:
    case State::LAND:
        break;
    case State::WALK:
        break;
    case State::FALL:
    case State::JUMP:
        m_vel.z -= s_gravity * deltaTime;
        break;
    default:
        break;
    }

    m_pos += m_vel * deltaTime;
    m_actor->SetWorldMat(mat * Matrix4::CreateTranslation(m_pos));
}
#endif