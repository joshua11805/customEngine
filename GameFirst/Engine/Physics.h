//
// Created by JoshuaShin on 4/11/26.
//

#ifndef GAME_PHYSICS_H
#define GAME_PHYSICS_H
#pragma once
#include "EngineMath.h"

class CollisionBox;

class Physics
{
public:
    static Physics* Get() { return s_physics; }

    class AABB
    {
    public:
        AABB() = default;
        AABB(const Vector3& min, const Vector3& max) : min(min), max(max) {}
        Vector3 min;
        Vector3 max;
    };

    class LineSegment
    {
    public:
        LineSegment(const Vector3& start, const Vector3& end) : start(start), end(end) {}

        Vector3 start;
        Vector3 end;
    };

    Physics() = default;
    ~Physics() = default;
    static void SetInstance(Physics* p) { s_physics = p; }

    static bool Intersect(const AABB& a, const AABB& b, AABB* pOverlap=nullptr);
    static bool Intersect(const LineSegment& segment, const AABB& box, Vector3* pHitPoint=nullptr);
    static bool UnitTest();

    void AddObj(const CollisionBox* box) { m_collisionBoxes.push_back(box); }
    void RemoveObj(const CollisionBox* box);
    bool SegmentCast(const LineSegment& segment, Vector3* pHitPoint = nullptr);

private:
    static Physics* s_physics;
    //making physics static kind of like renderer so it can be easily accessed without having to be passed in wtih arguments

    std::vector<const CollisionBox*> m_collisionBoxes;
};


#endif //GAME_PHYSICS_H