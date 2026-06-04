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
    output.position = float4(vIn.position, 1.0f);
    output.uv       = vIn.uv;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    float4 color = DiffuseTexture.Sample(DiffuseSampler, pIn.uv);

    if (color.r >= 0.8f || color.g >= 0.8f || color.b >= 0.8f)
        return color;
    else
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
}