#pragma pack_matrix(row_major)

cbuffer CrosshairConstants : register(b0, space3)
{
    float2 c_screenSize;
    float2 _pad;
};

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

VOut VS(VIn vIn)
{
    VOut output;
    output.position = float4(vIn.position, 1.0f);
    output.uv       = vIn.uv;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    float2 pixelPos = pIn.uv * c_screenSize;
    float2 center   = c_screenSize * 0.5f;
    float2 d        = abs(pixelPos - center);

    // Crosshair geometry (in pixels from center)
    float gap       = 4.0f;  // empty gap around center dot
    float armLength = 14.0f; // total reach from center
    float thickness = 1.5f;  // half-thickness of each arm
    float outline   = 1.0f;  // dark border width

    bool inHArm = (d.x >= gap && d.x <= armLength) && (d.y <= thickness);
    bool inVArm = (d.y >= gap && d.y <= armLength) && (d.x <= thickness);

    bool inHOutline = (d.x >= gap - outline && d.x <= armLength + outline) && (d.y <= thickness + outline);
    bool inVOutline = (d.y >= gap - outline && d.y <= armLength + outline) && (d.x <= thickness + outline);

    if (inHArm || inVArm)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (inHOutline || inVOutline)
        return float4(0.0f, 0.0f, 0.0f, 0.75f);

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
