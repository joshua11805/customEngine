#include "Constants.hlsl"

// Light Contribution debug view.
// Shows only the accumulated diffuse lighting value (no texture, no specular),
// so you can see exactly what light is reaching each surface.

struct VIn
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float3 normalWS : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VOut VS(VIn vIn)
{
    VOut output;
    float4 worldPos  = mul(float4(vIn.position, 1.0f), c_modelToWorld);
    output.position  = mul(worldPos, c_viewProj);
    output.normalWS  = mul(vIn.normal, (float3x3)c_modelToWorld);
    output.worldPos  = worldPos.xyz;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    float3 n     = normalize(pIn.normalWS);
    float3 accum = c_ambient;

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        if (!c_pointLight[i].isEnabled) continue;

        float3 lToVec = pIn.worldPos - c_pointLight[i].position;
        float  dist   = length(lToVec);
        if (dist >= c_pointLight[i].outerRadius) continue;

        float3 lightDir = -lToVec / max(dist, 0.0001f);
        float  nDotL    = max(dot(n, lightDir), 0.0f);
        float  att      = 1.0f - smoothstep(c_pointLight[i].innerRadius,
                                            c_pointLight[i].outerRadius, dist);

        // Diffuse only — no specular, no texture
        accum += c_pointLight[i].lightColor * (c_diffuseColor * nDotL * att);
    }

    return float4(accum, 1.0f);
}
