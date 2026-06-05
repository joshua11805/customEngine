//
// Created by JoshuaShin on 3/19/26.
//
#pragma once
#include "Actor.h"

/*
*     float3 position : POSITION0;
	float3 normal : NORMAL0;
	uint4 boneIndex : BLENDINDICES0;
	float4 weight : BLENDWEIGHT0;
	float2 uv : TEXCOORD0;
 */
class SkinnedObj : public Actor
{
public:
	static constexpr int MAX_SKELETON_BONES = 80;

	struct SkinConstants
	{
		Matrix4 c_skinMatrix[MAX_SKELETON_BONES];
	};
	SkinnedObj(Mesh* mesh);
	friend class Game;
	void Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, Shader* shaderOverride = nullptr) override;

	SkinConstants m_skinConstants;
};
