#pragma once
#include "EngineMath.h"

// This file contains C++ structures describing the various vertex formats

// For now, I have provided the first one
// This matches the HLSL structure from Simple.hlsl:
//struct VIn
//{
//    float3 position : POSITION0;
//    float4 color : COLOR0;
//};
struct VertexPosColor
{
    Vector3 pos;
    Color4 color;
};

struct VertexPosColorUV
{
    Vector3 pos;
    Color4 color;
    Vector2 uv;
};

struct VertexPosColorUVNormal
{
    Vector3 pos;
    Color4 color;
    Vector2 uv;
    Vector3 normal;
};

struct VertexPosNormalColorUV
{
    Vector3 pos;
    Vector3 normal;
    Color4 color;
    Vector2 uv;
};

struct SkinnedVertex
{
    Vector3  pos;
    Vector3  normal;
    uint8_t  boneIndices[4];
    uint8_t  boneWeights[4];
    Vector2  uv;
};

struct VertexPosNormalUV
{
    Vector3 pos;
    Vector3 normal;
    Vector2 uv;
};

struct VertexPosNormalTangentUV
{
    Vector3 pos;
    Vector3 normal;
    Vector3 tangent;
    Vector2 uv;
};

struct VertexPosUV
{
    Vector3 pos;
    Vector2 uv;
};
