//
// Created by JoshuaShin on 3/19/26.
//

#include "Animation.h"
#include "BoneTransform.h"
#include "JsonUtil.h"
#include "Skeleton.h"

Animation* Animation::StaticLoad(const char* fileName, AssetManager* pAssetManager)
{
    Animation* pAnim = new Animation();
    if (false == pAnim->Load(fileName))
    {
        delete pAnim;
        return nullptr;
    }
    return pAnim;
}

bool Animation::Load(const char* fileName)
{
    //copying first part from itpskel should be the same
    std::ifstream file(fileName);
    if (!file.is_open())
        return false;
    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    if (!doc.IsObject())
        return false;
    std::string str = doc["metadata"]["type"].GetString();
    int ver = doc["metadata"]["version"].GetInt();
    // Check the metadata
    if (!doc["metadata"].IsObject() || str != "itpanim" || ver != 2)
        return false;
    //sequence
    const rapidjson::Value& seq = doc["sequence"];
    if (!seq.IsObject())
        return false;
    m_isLoop = seq["loop"].GetBool();
    m_numFrames = seq["frames"].GetUint();
    m_animLength = seq["length"].GetFloat();
    m_numBones = seq["bonecount"].GetUint();

    m_tracks.resize(m_numBones);

    const rapidjson::Value& tracksArray = seq["tracks"];
    if (!tracksArray.IsArray())
        return false;

    for (rapidjson::SizeType i = 0; i < tracksArray.Size(); ++i)
    {
        //follows track -> bone / trans | trans -> rotation / translation
        //get the bone number
        const rapidjson::Value& track = tracksArray[i];
        uint32_t boneIndex = track["bone"].GetUint();
        //transforms array
        const rapidjson::Value& transforms = track["transforms"];
        m_tracks[boneIndex].reserve(transforms.Size());

        for (rapidjson::SizeType j = 0; j < transforms.Size(); ++j)
        {
            const rapidjson::Value& transform = transforms[j];
            BoneTransform boneTrans;

            //rotation
            const rapidjson::Value& rotationArray = transform["rot"];
            boneTrans.m_rotation = Quaternion(
                rotationArray[0].GetFloat(), //x
                rotationArray[1].GetFloat(), //y
                rotationArray[2].GetFloat(), //z
                rotationArray[3].GetFloat() //w
            );
            //translation
            const rapidjson::Value& translationArray = transform["trans"];
            boneTrans.m_translation = Vector3(
                translationArray[0].GetFloat(),
                translationArray[1].GetFloat(),
                translationArray[2].GetFloat()
            );

            m_tracks[boneIndex].push_back(boneTrans);
        }
    }
    return true;
}

void Animation::GetGlobalPoseAtTime(std::vector<Matrix4>& outPoses, const Skeleton* inSkeleton, float inTime) const
{
    const size_t numBones = inSkeleton->GetNumBones();
    outPoses.resize(numBones);

    //loop through all bones
    for (size_t i = 0; i < numBones; ++i)
    {
        const Skeleton::Bone& bone = inSkeleton->GetBone(i);
        Matrix4 localPose;

        //if empty use identity matrix
        if (m_tracks[i].empty())
        {
            localPose = Matrix4::Identity;
        }
        //convert bonetransform to matrix with ToMatrix
        else
        {
            //calculate correct keyframe
            float timePerFrame = m_animLength / (float)(m_numFrames);
            //find closest frame to intime
            int frame = (int)(inTime / timePerFrame + 0.5f);
            //clamp to valid range
            frame = Math::Clamp(frame, 0, (int)m_tracks[i].size() - 1);

            int frameb = frame + 1;
            frameb = Math::Clamp(frame, 0, (int)m_tracks[i].size() - 1);

            float f = (inTime - frame * timePerFrame) / timePerFrame;

            BoneTransform interpBT = BoneTransform::Interpolate(m_tracks[i][frame], m_tracks[i][frameb], f);


            localPose = interpBT.ToMatrix();
        }

        //combine with parent's global pose
        if (bone.m_parentIndex < 0) // back in global space
        {
            outPoses[i] = localPose;
        }
        else
        {
            outPoses[i] = localPose * outPoses[bone.m_parentIndex];
        }
    }
}
