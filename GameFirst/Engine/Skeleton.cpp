//
// Created by JoshuaShin on 3/19/26.
//

#include "Skeleton.h"
#include "AssetManager.h"
#include "JsonUtil.h"


void Skeleton::ComputeGlobalInvBindPose()
{
    const size_t numBones = m_bones.size();
    std::vector<Matrix4> globalBindPoses(numBones);
    //first loop: loop thorugh all the bones inverting the bind pose of each
    //since loop is starting from 0 we are starting from the root all the way down to the children
    for (size_t i = 0; i < numBones; ++i)
    {
        Matrix4 boneToParent = m_bones[i].m_bindPose.ToMatrix();
        //if root
        if (m_bones[i].m_parentIndex < 0)
        {
            globalBindPoses[i] = boneToParent;
        }
        else
        {
            //multiply by parent's bind pose matrix
            globalBindPoses[i] = boneToParent * globalBindPoses[m_bones[i].m_parentIndex];
        }
    }
    //second loop: invert each global bind pose to get global inverse
    m_GIBPM = globalBindPoses;
    for (size_t i = 0; i < numBones; ++i)
    {
        m_GIBPM[i].Invert();
    }

}

Skeleton* Skeleton::StaticLoad(const char* fileName, AssetManager* pAssetManager)
{
    Skeleton* pSkeleton = new Skeleton();
    if (false == pSkeleton->Load(fileName))
    {
        delete pSkeleton;
        return nullptr;
    }
    return pSkeleton;
}

bool Skeleton::Load(const char* fileName) {
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
    if (!doc["metadata"].IsObject() || str != "itpskel" || ver != 1)
        return false;
    const rapidjson::Value& bonecount = doc["bonecount"];
    if (!bonecount.IsUint())
        return false;
    size_t count = bonecount.GetUint();
    m_bones.reserve(count);
    const rapidjson::Value& bones = doc["bones"];
    if (!bones.IsArray())
        return false;
    if (bones.Size() != count)
        return false;
    Bone temp;
    for (rapidjson::SizeType i = 0; i < count; i++) {
        if (!bones[i].IsObject())
            return false;
        const rapidjson::Value& name = bones[i]["name"];
        temp.m_name = name.GetString();
        const rapidjson::Value& parent = bones[i]["parent"];
        temp.m_parentIndex = parent.GetInt();
        const rapidjson::Value& bindpose = bones[i]["bindpose"];
        if (!bindpose.IsObject())
            return false;
        if (false == GetQuaternionFromJSON(bindpose, "rot", temp.m_bindPose.m_rotation))
            return false;
        if (false == GetVectorFromJSON(bindpose, "trans", temp.m_bindPose.m_translation))
            return false;
        m_bones.push_back(temp);
    }
    // Now that we have the bones
    ComputeGlobalInvBindPose();
    return true;
}

