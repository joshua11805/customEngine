#include "Constants.hlsl"

// Vertex Color debug view.
// Requires meshes that carry a COLOR0 vertex attribute (e.g. those loaded via
// VertexPosNormalColorUV or the glTF COLOR_0 channel).
// Meshes using VertexPosNormalUV (most engine primitives) lack a vertex color
// channel — this shader is only useful once you have assets that provide one.
//
// HOW TO ACTIVATE:
//   1. Make sure your mesh vertex format includes a float4 COLOR0 attribute.
//   2. In Game::LoadShaders(), uncomment the "SceneVertexColor" shader block.
//   3. In Game::RenderFrame() Pass 1, the SceneViewMode::VertexColor case will
//      then bind "SceneVertexColor" instead of falling back to "SceneUnlit".

struct VIn
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float4 color    : COLOR0;   // vertex color — must exist in the vertex buffer
    float2 uv       : TEXCOORD0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
};

VOut VS(VIn vIn)
{
    VOut output;
    float4 worldPos = mul(float4(vIn.position, 1.0f), c_modelToWorld);
    output.position = mul(worldPos, c_viewProj);
    output.color    = vIn.color;
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return pIn.color;
}
