//
// Created by JoshuaShin on 3/19/26.
//

#include "BoneTransform.h"

Matrix4 BoneTransform::ToMatrix() const
{
    //create matrix from the quat and transform
    Matrix4 rot = Matrix4::CreateFromQuaternion(m_rotation);
    Matrix4 trans = Matrix4::CreateTranslation(m_translation);
    return rot * trans;
}

BoneTransform BoneTransform::Interpolate(const BoneTransform& a, const BoneTransform& b, float f)
{
    BoneTransform result;
    result.m_rotation = Quaternion::Slerp(a.m_rotation, b.m_rotation, f);
    result.m_translation = Vector3::Lerp(a.m_translation, b.m_translation, f);
    return result;
}

