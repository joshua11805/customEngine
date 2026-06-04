#include "Constants.hlsl"

struct VIn
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float4 color  : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};


VOut VS(VIn vIn)
{
    VOut output;

    float4 localPos = float4(vIn.position, 1.0f);
    float4 worldPos = mul(localPos, c_modelToWorld);
    float4 cameraPos = mul(worldPos, c_viewProj);

    output.normalWS = mul(vIn.normal, (float3x3)c_modelToWorld);
    output.worldPos = worldPos.xyz;
    output.position = cameraPos;
    output.color = vIn.color;
    output.uv = vIn.uv;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    float3 n = normalize(pIn.normalWS);
    float3 accum = c_ambient;

    for(int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        if(!c_pointLight[i].isEnabled)
            continue;

        float3 lToVec = pIn.worldPos - c_pointLight[i].position;
        float dist = length(lToVec);

        if(dist >= c_pointLight[i].outerRadius)
            continue;

        float3 lightDirection = -lToVec / max(dist, 0.0001f);
        //diffuse
        float nDotL = max(dot(n, lightDirection), 0.0f);

        //specular
        float3 viewDir = normalize(c_cameraPosition - pIn.worldPos);
        float3 halfVec = normalize(lightDirection + viewDir);
        float nDotH = max(dot(n, halfVec), 0.0f);
        float spec = pow(nDotH, c_specularPower);

        //attenuate
        float att = 1.0f - smoothstep(c_pointLight[i].innerRadius, c_pointLight[i].outerRadius, dist);

        accum += c_pointLight[i].lightColor * ((c_diffuseColor * nDotL + c_specularColor * spec) * att);
    }
    float4 tex = DiffuseTexture.Sample(DiffuseSampler, pIn.uv);
    float4 surfaceColor = tex * pIn.color;
    return float4(accum * surfaceColor.rgb, surfaceColor.a);
}