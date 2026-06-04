//
// Created by JoshuaShin on 4/11/26.
//

#include "Physics.h"
#include <cfloat>
#include "CollisionBox.h"
static constexpr float EPSILON = 1.0e-6f;
static constexpr float MAX = FLT_MAX;
Physics* Physics::s_physics = nullptr;


void Physics::RemoveObj(const CollisionBox* box)
{
    auto it = std::find(m_collisionBoxes.begin(), m_collisionBoxes.end(), box);
    if (it != m_collisionBoxes.end())
    {
        m_collisionBoxes.erase(it);
    }
}

bool Physics::SegmentCast(const LineSegment& segment, Vector3* pHitPoint)
{
    bool hit = false;
    float closestDistSq = MAX;
    Vector3 closestHit;

    for (const CollisionBox* box : m_collisionBoxes)
    {
        AABB aabb = box->GetAABB();
        Vector3 hitPoint;

        if (Intersect(segment, aabb, &hitPoint))
        {
            // find the hit closest to the segment start
            float distSq = (hitPoint - segment.start).LengthSq();
            if (distSq < closestDistSq)
            {
                closestDistSq = distSq;
                closestHit = hitPoint;
                hit = true;
            }
        }
    }

    if (hit && pHitPoint != nullptr)
        *pHitPoint = closestHit;

    return hit;
}



bool Physics::Intersect(const AABB& a, const AABB& b, AABB* pOverlap)
{
    //first check that they do not intersect
    if (a.max.x < b.min.x || b.max.x < a.min.x) return false;
    if (a.max.y < b.min.y || b.max.y < a.min.y) return false;
    if (a.max.z < b.min.z || b.max.z < a.min.z) return false;

    //if overlap
    if (pOverlap != nullptr)
    {
        pOverlap->min.x = std::max(a.min.x, b.min.x);
        pOverlap->min.y = std::max(a.min.y, b.min.y);
        pOverlap->min.z = std::max(a.min.z, b.min.z);

        pOverlap->max.x = std::min(a.max.x, b.max.x);
        pOverlap->max.y = std::min(a.max.y, b.max.y);
        pOverlap->max.z = std::min(a.max.z, b.max.z);
    }

    return true;
}

bool Physics::Intersect(const LineSegment& segment, const AABB& box, Vector3* pHitPoint)
{
    Vector3 d = segment.end - segment.start;
    float tmin = 0.0f;
    float tmax = 1.0f;  // 1.0f clamps to segment (not infinite ray)
    //rolled out the for loop since our Vector uses x,y,z not indexes
    // X axis
    if (fabsf(d.x) < EPSILON)
    {
        //ray is parallel to slab, no hit if origin isn't within in slab
        if (segment.start.x < box.min.x || segment.start.x > box.max.x)
            return false;
    }
    else
    {
        float ood = 1.0f / d.x;
        float t1 = (box.min.x - segment.start.x) * ood;
        float t2 = (box.max.x - segment.start.x) * ood;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Y axis
    if (fabsf(d.y) < EPSILON)
    {
        if (segment.start.y < box.min.y || segment.start.y > box.max.y)
            return false;
    }
    else
    {
        float ood = 1.0f / d.y;
        float t1 = (box.min.y - segment.start.y) * ood;
        float t2 = (box.max.y - segment.start.y) * ood;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Z axis
    if (fabsf(d.z) < EPSILON)
    {
        if (segment.start.z < box.min.z || segment.start.z > box.max.z)
            return false;
    }
    else
    {
        float ood = 1.0f / d.z;
        float t1 = (box.min.z - segment.start.z) * ood;
        float t2 = (box.max.z - segment.start.z) * ood;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    if (pHitPoint != nullptr)
        *pHitPoint = segment.start + d * tmin;

    return true;
}

bool Physics::UnitTest()
{
    struct TestAABB
    {
        AABB a;
        AABB b;
        AABB overlap;
    };

    const TestAABB testAABB[] =
    {
        {
            AABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f)),
            AABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f))
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-110.0f, -10.0f, -10.0f), Vector3(-90.0f, 10.0f, 10.0f)),
            AABB(Vector3(-100.0f, -10.0f, -10.0f), Vector3(-90.0f, 10.0f, 10.0f))
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(90.0f, -10.0f, -10.0f), Vector3(110.0f, 10.0f, 10.0f)),
            AABB(Vector3(90.0f, -10.0f, -10.0f), Vector3(100.0f, 10.0f, 10.0f))
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, -110.0f, -10.0f), Vector3(10.0f, -90.0f, 10.0f)),
            AABB(Vector3(-10.0f, -100.0f, -10.0f), Vector3(10.0f, -90.0f, 10.0f))
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, 90.0f, -10.0f), Vector3(10.0f, 110.0f, 10.0f)),
            AABB(Vector3(-10.0f, 90.0f, -10.0f), Vector3(10.0f, 100.0f, 10.0f))
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, -10.0f, -110.0f), Vector3(10.0f, 10.0f, -90.0f)),
            AABB(Vector3(-10.0f, -10.0f, -100.0f), Vector3(10.0f, 10.0f, -90.0f))
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, -10.0f, 90.0f), Vector3(10.0f, 10.0f, 110.0f)),
            AABB(Vector3(-10.0f, -10.0f, 90.0f), Vector3(10.0f, 10.0f, 100.0f))
        },
        // Non-intersecting cases — overlap is intentionally impossible (min > max)
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-120.0f, -10.0f, -10.0f), Vector3(-110.0f, 10.0f, 10.0f)),
            AABB(Vector3::One, Vector3::Zero)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(110.0f, -10.0f, -10.0f), Vector3(120.0f, 10.0f, 10.0f)),
            AABB(Vector3::One, Vector3::Zero)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, -120.0f, -10.0f), Vector3(10.0f, -110.0f, 10.0f)),
            AABB(Vector3::One, Vector3::Zero)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, 110.0f, -10.0f), Vector3(10.0f, 120.0f, 10.0f)),
            AABB(Vector3::One, Vector3::Zero)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, -10.0f, -120.0f), Vector3(10.0f, 10.0f, -110.0f)),
            AABB(Vector3::One, Vector3::Zero)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            AABB(Vector3(-10.0f, -10.0f, 110.0f), Vector3(10.0f, 10.0f, 120.0f)),
            AABB(Vector3::One, Vector3::Zero)
        },
    };

    for (int i = 0; i < std::size(testAABB); i++)
    {
        const TestAABB& t = testAABB[i];
        //check for non-intersect cases
        bool shouldIntersect (t.overlap.min.x <= t.overlap.max.x &&
                              t.overlap.min.y <= t.overlap.max.y &&
                              t.overlap.min.z <= t.overlap.max.z);
        AABB overlap;
        bool result = Intersect(t.a, t.b, &overlap);
        //case where boxes should not be intersecting but return true
        if (result != shouldIntersect)
        {
            SDL_Log("AABB test %d failed: expected %s but got %s",
                i,
                shouldIntersect ? "true" : "false",
                result ? "true" : "false");
            return false;
        }

        if (shouldIntersect)
        {
            if (!Vector3::IsCloseEnuf(overlap.min, t.overlap.min) ||
                !Vector3::IsCloseEnuf(overlap.max, t.overlap.max))
            {
                SDL_Log("AABB test %d overlap mismatch!", i);
                SDL_Log("  Expected min: (%.2f, %.2f, %.2f) max: (%.2f, %.2f, %.2f)",
                    t.overlap.min.x, t.overlap.min.y, t.overlap.min.z,
                    t.overlap.max.x, t.overlap.max.y, t.overlap.max.z);
                SDL_Log("  Got     min: (%.2f, %.2f, %.2f) max: (%.2f, %.2f, %.2f)",
                    overlap.min.x, overlap.min.y, overlap.min.z,
                    overlap.max.x, overlap.max.y, overlap.max.z);
                return false;
            }
        }
    }
    SDL_Log("All AABB tests passed");

    //Linesegment tests
    struct TestSegment
    {
        AABB box;
        LineSegment segment;
        bool hit;
        Vector3 point;
    };

    const TestSegment testSegment[] =
    {
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(-110.0f, 0.0f, 0.0f), Vector3(-90.0f, 0.0f, 0.0f)),
            true, Vector3(-100.0f, 0.0f, 0.0f)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, -110.0f, 0.0f), Vector3(0.0f, -90.0f, 0.0f)),
            true, Vector3(0.0f, -100.0f, 0.0f)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, 0.0f, -110.0f), Vector3(0.0f, 0.0f, -90.0f)),
            true, Vector3(0.0f, 0.0f, -100.0f)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(110.0f, 0.0f, 0.0f), Vector3(90.0f, 0.0f, 0.0f)),
            true, Vector3(100.0f, 0.0f, 0.0f)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, 110.0f, 0.0f), Vector3(0.0f, 90.0f, 0.0f)),
            true, Vector3(0.0f, 100.0f, 0.0f)
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, 0.0f, 110.0f), Vector3(0.0f, 0.0f, 90.0f)),
            true, Vector3(0.0f, 0.0f, 100.0f)
        },
        // Non-hitting cases
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(-120.0f, 0.0f, 0.0f), Vector3(-110.0f, 0.0f, 0.0f)),
            false, Vector3::Zero
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, -120.0f, 0.0f), Vector3(0.0f, -110.0f, 0.0f)),
            false, Vector3::Zero
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, 0.0f, -120.0f), Vector3(0.0f, 0.0f, -110.0f)),
            false, Vector3::Zero
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(120.0f, 0.0f, 0.0f), Vector3(110.0f, 0.0f, 0.0f)),
            false, Vector3::Zero
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, 120.0f, 0.0f), Vector3(0.0f, 110.0f, 0.0f)),
            false, Vector3::Zero
        },
        {
            AABB(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f)),
            LineSegment(Vector3(0.0f, 0.0f, 120.0f), Vector3(0.0f, 0.0f, 110.0f)),
            false, Vector3::Zero
        },
    };

    for (int i = 0; i < std::size(testSegment); i++)
    {
        const TestSegment& t = testSegment[i];
        Vector3 hitPoint;
        bool result = Intersect(t.segment, t.box, &hitPoint);
        //if there is a hit when there is not supposed to be
        if (result != t.hit)
        {
            SDL_Log("LineSegment test %d failed: expected %s but got %s",
                i,
                t.hit ? "true" : "false",
                result ? "true" : "false");
            return false;
        }

        if (t.hit)
        {
            //if hit is correct then check that the intersection point is close enough
            if (!Vector3::IsCloseEnuf(hitPoint, t.point))
            {
                SDL_Log("LineSegment test %d hit point mismatch!", i);
                SDL_Log("  Expected: (%.2f, %.2f, %.2f)",
                    t.point.x, t.point.y, t.point.z);
                SDL_Log("  Got:      (%.2f, %.2f, %.2f)",
                    hitPoint.x, hitPoint.y, hitPoint.z);
                return false;
            }
        }
    }
    SDL_Log("All LineSegment tests passed!");
    return true;
}


