#pragma once

class Mesh;
class AssetManager;

// Generates simple procedural meshes using the Phong (VertexPosNormalUV) pipeline.
// The returned Mesh is heap-allocated; caller takes ownership.
namespace PrimitiveMesh
{
    Mesh* CreateCube  (AssetManager* assets, float halfSize = 50.f);
    Mesh* CreateSphere(AssetManager* assets, float radius   = 50.f,
                       int stacks = 16, int slices = 16);
}
