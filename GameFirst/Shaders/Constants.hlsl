// We want to use row major matrices
#pragma pack_matrix(row_major)

cbuffer PerObjectConstants : register(b0, space1)
{
    float4x4 c_modelToWorld;
};

cbuffer PerCameraConstants : register(b1, space1)
{
    float4x4 c_viewProj;
};

cbuffer MaterialConstants : register(b0, space3)
{
    float3 c_diffuseColor;
    float3 c_specularColor;
    float c_specularPower;
};

#define MAX_POINT_LIGHTS 8
struct PointLightData
{
    float3 lightColor;
    float _pad0;
    float3 position;
    float _pad1;
    float innerRadius;
    float outerRadius;
    bool isEnabled;
    float _pad2;
};

cbuffer LightingConstants : register(b1, space3)
{
    float3 c_cameraPosition;
    float3 c_ambient;
    PointLightData c_pointLight[MAX_POINT_LIGHTS];
};

SamplerState DiffuseSampler : register(s0, space2);
Texture2D DiffuseTexture : register(t0, space2);

//normal map texture and sampler in slot 1
SamplerState NormalSampler : register(s1, space2);
Texture2D NormalTexture : register(s1, space2);
