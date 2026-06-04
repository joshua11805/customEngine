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

    output.position = float4(vIn.position, 1.0);
    output.color = vIn.color;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return pIn.color;
}
