#include "Constants.hlsl"
#include "Lighting.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
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

    float4 localPos = float4(vIn.position, 1.0f);
    float4 worldPos = mul(localPos, c_modelToWorld);
    float4 cameraPos = mul(worldPos, c_viewProj);

    output.normalWS = mul(vIn.normal, (float3x3)c_modelToWorld);
    output.worldPos = worldPos.xyz;
    output.position = cameraPos;
    output.uv = vIn.uv;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
	return CalcLighting(pIn.normalWS, pIn.worldPos, pIn.uv);
}