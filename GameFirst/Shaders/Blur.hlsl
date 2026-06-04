#include "Constants.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float2 uv       : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

cbuffer BlurConstants : register(b0, space3)
{
    float2 c_blurDir;
    float2 c_texelSize;  // 1.0 / texture dimensions
};

// Gaussian weights and offsets for 9-tap blur
// using linear sampling trick to reduce to 5 samples
static const float weight[3] = { 0.2270270270f, 0.3162162162f, 0.0702702703f };
static const float offset[3] = { 0.0f, 1.3846153846f, 3.2307692308f };

VOut VS(VIn vIn)
{
    VOut output;
    output.position = float4(vIn.position, 1.0f);
    output.uv       = vIn.uv;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    float2 texelSize = c_texelSize * c_blurDir;

    float4 result = DiffuseTexture.Sample(DiffuseSampler, pIn.uv) * weight[0];

    for (int i = 1; i < 3; i++)
    {
        result += DiffuseTexture.Sample(DiffuseSampler, pIn.uv + texelSize * offset[i]) * weight[i];
        result += DiffuseTexture.Sample(DiffuseSampler, pIn.uv - texelSize * offset[i]) * weight[i];
    }

    return result;
}