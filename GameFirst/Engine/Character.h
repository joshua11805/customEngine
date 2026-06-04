//
// Created by JoshuaShin on 3/20/26.
//
#pragma once
#include "Component.h"
#include "JobManager.h"
#include <unordered_map>
#include <string>

#include "JobManager.h"

class SkinnedObj;
class Skeleton;
class Animation;
class AssetManager;

class Character : public Component
{
public:
	//creating AnimJob as a child of Job, but making it here for easy access to anim data
	class AnimJob : public JobManager::Job
	{
	public:
		AnimJob(Character* character) : m_character(character) {}
		void DoIt() override;
	private:
		Character* m_character;
	};
	Character(class Actor* owner, AssetManager* assetManager);
	friend class Actor;
	void LoadProperties(const rapidjson::Value& properties) override;
	void Update(float deltaTime) override;

	bool SetAnim(const std::string& animName);
	void UpdateAnim(float deltaTime);
	bool IsAnimDone() const;

protected:
	SkinnedObj* m_skinnedObj = nullptr;
	Skeleton* m_skeleton = nullptr;
	AssetManager* m_assetManager = nullptr;
	const Animation* m_currAnim = nullptr;

	std::unordered_map<std::string, const Animation*> m_animations;
	float m_animTime = 0.0f;
};

