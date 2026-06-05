#include "pch.h"
#include "PrimitiveMesh.h"
#include "Mesh.h"
#include "Material.h"
#include "AssetManager.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"
#include "Renderer.h"
#include "EngineMath.h"

#include <vector>
#include <cmath>

static Mesh* BuildMesh(std::vector<VertexPosNormalUV>& verts,
                       std::vector<uint16_t>& indices,
                       AssetManager* assets)
{
    Material* mat = new Material();
    mat->SetShader(assets->GetShader("Phong"));
    mat->SetDiffuseColor (Vector3(0.8f, 0.8f, 0.8f));
    mat->SetSpecularColor(Vector3(0.3f, 0.3f, 0.3f));
    mat->SetSpecularPower(16.f);

    VertexBuffer* vb = new VertexBuffer(
        Renderer::Get(),
        verts.data(),  static_cast<uint32_t>(verts.size()  * sizeof(VertexPosNormalUV)),
        indices.data(), static_cast<uint32_t>(indices.size()), sizeof(uint16_t)
    );

    Mesh* mesh = new Mesh(vb, mat);
    return mesh;
}

Mesh* PrimitiveMesh::CreateCube(AssetManager* assets, float halfSize)
{
    // 6 faces × 4 verts = 24 verts, 6 × 2 tris × 3 idx = 36 indices
    const Vector3 faceNormals[6] = {
        Vector3( 1, 0, 0), Vector3(-1, 0, 0),
        Vector3( 0, 1, 0), Vector3( 0,-1, 0),
        Vector3( 0, 0, 1), Vector3( 0, 0,-1),
    };
    // For each face, the 4 corners in CCW order (viewed from outside)
    // We build them analytically from the normal and two tangent axes.
    std::vector<VertexPosNormalUV> verts;
    std::vector<uint16_t> indices;
    verts.reserve(24);
    indices.reserve(36);

    for (int f = 0; f < 6; ++f)
    {
        Vector3 n = faceNormals[f];

        // Pick tangent and bitangent (perpendicular to n)
        Vector3 t, b;
        if (fabsf(n.x) < 0.9f)
            t = Vector3(1, 0, 0);
        else
            t = Vector3(0, 1, 0);
        // b = n × t, t = b × n  (right-hand, Z-up friendly)
        b = Vector3(n.y*t.z - n.z*t.y, n.z*t.x - n.x*t.z, n.x*t.y - n.y*t.x);
        t = Vector3(b.y*n.z - b.z*n.y, b.z*n.x - b.x*n.z, b.x*n.y - b.y*n.x);

        uint16_t base = static_cast<uint16_t>(verts.size());

        // 4 corners: center-of-face ± t ± b
        Vector3 center = n * halfSize;
        VertexPosNormalUV v0, v1, v2, v3;
        v0.pos = center - t*halfSize - b*halfSize; v0.normal = n; v0.uv = Vector2(0,0);
        v1.pos = center + t*halfSize - b*halfSize; v1.normal = n; v1.uv = Vector2(1,0);
        v2.pos = center + t*halfSize + b*halfSize; v2.normal = n; v2.uv = Vector2(1,1);
        v3.pos = center - t*halfSize + b*halfSize; v3.normal = n; v3.uv = Vector2(0,1);
        verts.push_back(v0); verts.push_back(v1); verts.push_back(v2); verts.push_back(v3);

        // Two tris per face (CCW from outside)
        indices.insert(indices.end(), { base, uint16_t(base+1), uint16_t(base+2),
                                        base, uint16_t(base+2), uint16_t(base+3) });
    }

    return BuildMesh(verts, indices, assets);
}

Mesh* PrimitiveMesh::CreateSphere(AssetManager* assets, float radius, int stacks, int slices)
{
    std::vector<VertexPosNormalUV> verts;
    std::vector<uint16_t> indices;
    verts.reserve((stacks + 1) * (slices + 1));
    indices.reserve(stacks * slices * 6);

    for (int stack = 0; stack <= stacks; ++stack)
    {
        float phi = Math::Pi * static_cast<float>(stack) / static_cast<float>(stacks);
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int slice = 0; slice <= slices; ++slice)
        {
            float theta = Math::TwoPi * static_cast<float>(slice) / static_cast<float>(slices);
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            Vector3 n(sinPhi * cosTheta, sinPhi * sinTheta, cosPhi);
            VertexPosNormalUV v;
            v.pos    = n * radius;
            v.normal = n;
            v.uv     = Vector2(static_cast<float>(slice) / slices,
                               static_cast<float>(stack) / stacks);
            verts.push_back(v);
        }
    }

    for (int stack = 0; stack < stacks; ++stack)
    {
        for (int slice = 0; slice < slices; ++slice)
        {
            uint16_t a = static_cast<uint16_t>( stack      * (slices + 1) + slice);
            uint16_t b = static_cast<uint16_t>( stack      * (slices + 1) + slice + 1);
            uint16_t c = static_cast<uint16_t>((stack + 1) * (slices + 1) + slice);
            uint16_t d = static_cast<uint16_t>((stack + 1) * (slices + 1) + slice + 1);
            indices.insert(indices.end(), { a, b, d,  a, d, c });
        }
    }

    return BuildMesh(verts, indices, assets);
}
