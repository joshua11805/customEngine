#pragma once

class AssetManager;

template<class T> class AssetCache
{
public:
    AssetCache(AssetManager* pManager)
        : mManager(pManager)
    {
    }

    ~AssetCache()
    {
        Clear();
    }

    T* Get(const std::string& fileName)
    {
        auto iter = mAssetMap.find(fileName);
        if (iter != mAssetMap.end())
            return iter->second;

        return nullptr;
    }

    T* Load(const std::string& fileName)
    {
        auto iter = mAssetMap.find(fileName);
        if (iter != mAssetMap.end())
            return iter->second;

        T* asset = T::StaticLoad(fileName.c_str(), mManager);
        if (nullptr != asset)
            mAssetMap[fileName] = asset;
        return asset;
    }

    void Cache(const std::string& key, T* asset)
    {
        mAssetMap[key] = asset;
    }

    void Clear()
    {
        for (auto asset : mAssetMap)
        {
            delete asset.second;
        }
        mAssetMap.clear();
    }

private:
    AssetManager* mManager;
    std::unordered_map<std::string, T*> mAssetMap;
};