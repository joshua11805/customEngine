#include "Constants.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float  height   : TEXCOORD0;
};

VOut VS(VIn vIn)
{
    VOut output;
    float4 worldPos4 = mul(float4(vIn.position, 1.0f), c_modelToWorld);
    output.position  = mul(worldPos4, c_viewProj);
    output.height    = vIn.position.z;
    return output;
}

float3 HeightColor(float z)
{
    float t = saturate(z / 500.0f);
    if (t < 0.10f) return float3(0.10f, 0.25f, 0.60f);
    if (t < 0.22f) return float3(0.80f, 0.75f, 0.50f);
    if (t < 0.50f) return float3(0.25f, 0.55f, 0.20f);
    if (t < 0.75f) return float3(0.45f, 0.38f, 0.28f);
    return             float3(0.90f, 0.92f, 0.95f);
}

float4 PS(VOut pIn) : SV_TARGET
{
    float3 color = HeightColor(pIn.height);
    return float4(color, 1.0f);
}
