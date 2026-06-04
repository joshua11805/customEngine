// UI.hlsl — Screen-space 2D rendering with alpha blending.
// Input vertices are in screen pixels (top-left origin, Y-down).
// Fragment shader multiplies texture sample by vertex color.
// For solid panels: bind a white 1x1 texture and set vertex color = tint.
// For text: bind the font atlas (stored as RGBA with alpha in all channels).

cbuffer UIScreenConstants : register(b0, space1) {
    float2 c_screenSize;
    float2 _pad;
};

SamplerState UISampler : register(s0, space2);
Texture2D    UITexture  : register(t0, space2);

struct VIn {
    float2 position : POSITION0;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

struct VOut {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

VOut VS(VIn vIn) {
    VOut output;
    output.position.x = (vIn.position.x / c_screenSize.x) * 2.0f - 1.0f;
    output.position.y = 1.0f - (vIn.position.y / c_screenSize.y) * 2.0f;
    output.position.z = 0.0f;
    output.position.w = 1.0f;
    output.uv    = vIn.uv;
    output.color = vIn.color;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET {
    float4 texColor = UITexture.Sample(UISampler, pIn.uv);
    return texColor * pIn.color;
}
