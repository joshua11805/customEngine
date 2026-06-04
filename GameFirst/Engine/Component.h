//
// Created by JoshuaShin on 3/4/26.
//

#pragma once
#ifndef GAME_COMPONENT_H
#define GAME_COMPONENT_H

class Actor;

class Component
{
public:
    Component(Actor* actor) { m_actor = actor; }
    virtual ~Component();
    virtual void LoadProperties(const rapidjson::Value& properties);
    virtual void Update(float deltaTime);

protected:
    Actor* m_actor;
};

#endif //GAME_COMPONENT_H
