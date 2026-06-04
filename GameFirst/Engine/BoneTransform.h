//
// Created by JoshuaShin on 3/19/26.
//

#ifndef GAME_BONETRANSFORM_H
#define GAME_BONETRANSFORM_H
#include "EngineMath.h"

class BoneTransform
{
public:
    Matrix4 ToMatrix() const;
    Quaternion m_rotation;
    Vector3 m_translation;
    static BoneTransform Interpolate(const BoneTransform& a, const BoneTransform& b, float f);
};


#endif //GAME_BONETRANSFORM_H