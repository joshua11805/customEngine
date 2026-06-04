//
// Created by JoshuaShin on 3/19/26.
//

#ifndef GAME_SKELETON_H
#define GAME_SKELETON_H
#include "BoneTransform.h"

class BoneTransform;
class AssetManager;

class Skeleton
{
public:
    struct Bone
    {
        BoneTransform m_bindPose;
        std::string m_name;
        int m_parentIndex;
    };
    //public accessor functions
    size_t GetNumBones() const { return m_bones.size(); }
    const Bone& GetBone(size_t index) const { return m_bones[index]; }
    const std::vector<Bone>& GetBones() const { return m_bones; }
    const std::vector<Matrix4>& GetGlobalInvBindPoses() const { return m_GIBPM; }
    static Skeleton* StaticLoad(const char* fileName, AssetManager* pAssetManager);

private:
    std::vector<Bone> m_bones;
    std::vector<Matrix4> m_GIBPM;
    void ComputeGlobalInvBindPose();
    bool Load(const char* fileName);
};


#endif //GAME_SKELETON_H