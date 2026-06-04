#include "Constants.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};


VOut VS(VIn vIn)
{
    VOut output;

    float4 localPos = float4(vIn.position, 1.0f);
    float4 worldPos = mul(localPos, c_modelToWorld);
    float4 cameraPos = mul(worldPos, c_viewProj);

    output.position = cameraPos;
    output.color = vIn.color;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return pIn.color;
}
