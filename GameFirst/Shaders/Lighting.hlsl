float4 CalcLighting(float3 normalWS, float3 worldPos, float2 uv)
{
    float3 n = normalize(normalWS);
    float3 accum = c_ambient;

    for(int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        if(!c_pointLight[i].isEnabled)
            continue;

        float3 lToVec = worldPos - c_pointLight[i].position;
        float dist = length(lToVec);

        if(dist >= c_pointLight[i].outerRadius)
            continue;

        float3 lightDirection = -lToVec / max(dist, 0.0001f);
        //diffuse
        float nDotL = max(dot(n, lightDirection), 0.0f);

        //specular
        float3 viewDir = normalize(c_cameraPosition - worldPos);
        float3 halfVec = normalize(lightDirection + viewDir);
        float nDotH = max(dot(n, halfVec), 0.0f);
        float spec = pow(nDotH, c_specularPower);

        //attenuate
        float att = 1.0f - smoothstep(c_pointLight[i].innerRadius, c_pointLight[i].outerRadius, dist);

        accum += c_pointLight[i].lightColor * ((c_diffuseColor * nDotL + c_specularColor * spec) * att);
    }
    float4 tex = DiffuseTexture.Sample(DiffuseSampler, uv);
    return float4(accum * tex.rgb, tex.a);
}