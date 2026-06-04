//
// Created by JoshuaShin on 3/20/26.
//
#include "Character.h"

#include "Animation.h"
#include "pch.h"
#include "SkinnedObj.h"
#include "AssetManager.h"
#include "JsonUtil.h"
#include "Skeleton.h"
#include "Profiler.h"
#include "JobManager.h"


Character::Character(class Actor* owner, AssetManager* assetManager) : Component(owner), m_assetManager(assetManager)
{
    m_skinnedObj = static_cast<SkinnedObj*>(owner);
}

bool Character::IsAnimDone() const
{
    return m_animTime >= m_currAnim->GetLength();
}


void Character::LoadProperties(const rapidjson::Value& properties)
{
    //load skeleton into asset manager
    std::string skeletonFile;
    GetStringFromJSON(properties, "skeleton", skeletonFile);
    m_skeleton = m_assetManager->LoadSkeleton(skeletonFile);

    //load animations into asset manager
    if (properties.HasMember("animations") && properties["animations"].IsArray())
    {
        const rapidjson::Value& animations = properties["animations"];
        for (rapidjson::SizeType i = 0; i < animations.Size(); ++i)
        {
            if (animations[i].IsArray())
            {
                std::string shortName = animations[i][0].GetString();
                std::string fileName = animations[i][1].GetString();

                Animation* anim = m_assetManager->LoadAnimation(fileName);
                if (anim)
                {
                    m_animations[shortName] = anim;
                }
            }
        }
    }
}

bool Character::SetAnim(const std::string& animName)
{
    //check if the anim exists
    auto it = m_animations.find(animName);
    if (it == m_animations.end())
        return false;

    //if it does set curr anim to the animation
    m_currAnim = it->second;
    m_animTime = 0.0f;
    return true;
}

void Character::Update(float deltaTime)
{
    //set default ot run if there is no anim
    if (!m_currAnim)
        SetAnim("run");

    UpdateAnim(deltaTime);
}

void Character::UpdateAnim(float deltaTime)
{
    PROFILE_SCOPE(UpdateAnim);
    m_animTime += deltaTime;

    float length = m_currAnim->GetLength();

    //if looping just wrap around
    if (m_currAnim->IsLooping())
    {
        if (m_animTime >= length)
        {
            m_animTime = fmodf(m_animTime, length);
        }
    }
    //if not looping make sure to clamp to length
    else
    {
        if (m_animTime > length)
        {
            m_animTime = length;
        }
    }
    JobManager::Get()->AddJob(new AnimJob(this));
}

void Character::AnimJob::DoIt()
{
    PROFILE_SCOPE(AnimJob);
    //copied the last part of UpdateAnim and made it into the AnimJob
    //get global pose matrices at curr time
    std::vector<Matrix4> poses;
    m_character->m_currAnim->GetGlobalPoseAtTime(poses, m_character->m_skeleton, m_character->m_animTime);

    //combine with GIBPM and copy into skin buffer
    const std::vector<Matrix4>& invertedBindPoses = m_character->m_skeleton->GetGlobalInvBindPoses();
    for (size_t i = 0; i < poses.size(); ++i)
    {
        m_character->m_skinnedObj->m_skinConstants.c_skinMatrix[i] = invertedBindPoses[i] * poses[i];
    }
}

