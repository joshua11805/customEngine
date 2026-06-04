#include "Constants.hlsl"
#include "Lighting.hlsl"
#define MAX_SKELETON_BONES 80
cbuffer SkinConstants : register(b2, space1)
{
    float4x4 c_skinMatrix[MAX_SKELETON_BONES];
}

struct VIn
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    uint4 boneIndex : BLENDINDICES0;
    float4 weight : BLENDWEIGHT0;
    float2 uv : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};


VOut VS(VIn vIn)
{
    VOut output;

    float4 skinnedPos    = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);

    for(int i = 0; i < 4; ++i)
    {
        float4x4 skinMat = c_skinMatrix[vIn.boneIndex[i]];
        float w = vIn.weight[i];

        skinnedPos += w * mul(float4(vIn.position, 1.0f), skinMat);
        skinnedNormal += w * mul(vIn.normal, (float3x3)skinMat);
    }

    float4 worldPos = mul(skinnedPos, c_modelToWorld);
    float4 cameraPos = mul(worldPos, c_viewProj);

    output.normalWS = mul(skinnedNormal, (float3x3)c_modelToWorld);
    output.worldPos = worldPos.xyz;
    output.position = cameraPos;
    output.uv = vIn.uv;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return CalcLighting(pIn.normalWS, pIn.worldPos, pIn.uv);
}