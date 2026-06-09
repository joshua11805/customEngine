#include "Constants.hlsl"

#define MAX_SKELETON_BONES 80
cbuffer SkinConstants : register(b2, space1)
{
    float4x4 c_skinMatrix[MAX_SKELETON_BONES];
}

struct VIn
{
    float3 position  : POSITION0;
    float3 normal    : NORMAL0;
    uint4  boneIndex : BLENDINDICES0;
    float4 weight    : BLENDWEIGHT0;
    float2 uv        : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
};

VOut VS(VIn vIn)
{
    VOut output;

    float4 skinnedPos = float4(0, 0, 0, 0);
    for (int i = 0; i < 4; ++i)
    {
        float4x4 skinMat = c_skinMatrix[vIn.boneIndex[i]];
        skinnedPos += vIn.weight[i] * mul(float4(vIn.position, 1.0f), skinMat);
    }

    float4 worldPos  = mul(skinnedPos, c_modelToWorld);
    output.position  = mul(worldPos, c_viewProj);
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return float4(0.85f, 0.85f, 0.85f, 1.0f);
}
