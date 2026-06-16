#include "Constants.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float4 color    : COLOR0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
};

VOut VS(VIn vIn)
{
    VOut output;
    float4 worldPos  = mul(float4(vIn.position, 1.0f), c_modelToWorld);
    output.position  = mul(worldPos, c_viewProj);
    output.color     = vIn.color;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return pIn.color;
}
