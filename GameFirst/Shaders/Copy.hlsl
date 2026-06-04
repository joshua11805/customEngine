#include "Constants.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float2 uv    : TEXCOORD0;
};


VOut VS(VIn vIn)
{
    VOut output;
    // no transformation, vertices are already in camera space
    output.position = float4(vIn.position, 1.0f);
    output.uv       = vIn.uv;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return DiffuseTexture.Sample(DiffuseSampler, pIn.uv);
}