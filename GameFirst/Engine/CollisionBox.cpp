//
// Created by JoshuaShin on 4/11/26.
//
#include "pch.h"
#include "CollisionBox.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "Physics.h"

CollisionBox::CollisionBox(Actor* owner)
    : Component(owner)
{
    // add itself physics
    Physics::Get()->AddObj(this);
}

CollisionBox::~CollisionBox()
{
    // remove itself from physics
    Physics::Get()->RemoveObj(this);
}

void CollisionBox::LoadProperties(const rapidjson::Value& properties)
{
    Vector3 min, max;
    GetVectorFromJSON(properties, "min", min);
    GetVectorFromJSON(properties, "max", max);
    m_aabb = Physics::AABB(min, max);
}

const Physics::AABB CollisionBox::GetAABB() const
{
    // actor's world matrix
    Matrix4 mat = m_actor->GetWorldMat().c_modelToWorld;
    float scale = mat.GetScale().x; // extract scale
    Vector3 translation = mat.GetTranslation(); //extract translation
    Physics::AABB result;
    //applu scale and translatoin to a copy of AABB
    result.min = m_aabb.min * scale + translation;
    result.max = m_aabb.max * scale + translation;

    return result;
}
