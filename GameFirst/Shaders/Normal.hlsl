#include "Constants.hlsl"
#include "Lighting.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 tangentWS  : TEXCOORD3;
};


VOut VS(VIn vIn)
{
    VOut output;

    float4 localPos = float4(vIn.position, 1.0f);
    float4 worldPos = mul(localPos, c_modelToWorld);
    float4 cameraPos = mul(worldPos, c_viewProj);

    output.normalWS = mul(vIn.normal, (float3x3)c_modelToWorld);
    output.tangentWS = mul(vIn.tangent.xyz, (float3x3)c_modelToWorld);
    output.worldPos = worldPos.xyz;
    output.position = cameraPos;
    output.uv = vIn.uv;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    //normalize normal and tangent
    float3 normal = normalize(pIn.normalWS);
    float3 tangent = normalize(pIn.tangentWS);

    //compute bitangent (cross product of normal and tangent
    float3 bit = normalize(cross(normal, tangent));

    //sample normal map and unpack from [0,1] to [-1, 1]
    float3 normalSample = NormalTexture.Sample(NormalSampler, pIn.uv).xyz;
    normalSample = normalSample * 2.0f - 1.0f;

    //transform normal from tangent space to world space using matrix
    float3 mappedNormal = normalize(normalSample.x * tangent + normalSample.y * bit + normalSample.z * normal);

    return CalcLighting(mappedNormal, pIn.worldPos, pIn.uv);
}