//
// Created by JoshuaShin on 3/19/26.
//

#ifndef GAME_ANIMATION_H
#define GAME_ANIMATION_H

class BoneTransform;
class AssetManager;
class Skeleton;
class Matrix4;

class Animation
{
public:
    uint32_t GetNumBones() const { return m_numBones; }
    uint32_t GetNumFrames() const { return m_numFrames; }
    float GetLength() const { return m_animLength; }
    static Animation* StaticLoad(const char* fileName, AssetManager* pAssetManager);
    void GetGlobalPoseAtTime(std::vector<Matrix4>& outPoses, const Skeleton* inSkeleton, float inTime) const;
    bool IsLooping() const { return m_isLoop; }
private:
    bool Load(const char* fileName);

    bool m_isLoop = false;
    uint32_t m_numBones = 0;
    uint32_t m_numFrames = 0;
    float m_animLength = 0;
    std::vector<std::vector<BoneTransform>> m_tracks;
};


#endif //GAME_ANIMATION_H