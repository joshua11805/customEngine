#include "Constants.hlsl"

// Wireframe-on-Shaded overlay pass.
// Drawn additively on top of the Lit pass in a second sub-pass (no clear).
// PS outputs a dim white so the lines are visible but don't blow out the lit surface.

struct VIn
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
};

VOut VS(VIn vIn)
{
    VOut output;
    float4 worldPos  = mul(float4(vIn.position, 1.0f), c_modelToWorld);
    output.position  = mul(worldPos, c_viewProj);
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return float4(0.4f, 0.4f, 0.4f, 1.0f);
}
