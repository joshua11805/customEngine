//
// Created by JoshuaShin on 4/11/26.
//

#pragma once
#include "Component.h"
#include "Physics.h"

class CollisionBox : public Component
{
public:
    CollisionBox(Actor* owner);
    ~CollisionBox() override;

    void LoadProperties(const rapidjson::Value& properties) override;
    const Physics::AABB GetAABB() const;
private:
    Physics::AABB m_aabb;
};

