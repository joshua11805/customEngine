#pragma once

class Mesh;
class AssetManager;

// Phase 1: static-mesh import from glTF/GLB files.
//
// Parses a .glb/.gltf via cgltf and populates an existing Mesh with a single
// merged vertex/index buffer (VertexPosNormalUV) plus a Phong material mapped
// from the file's first PBR material. Skinning and animation are not handled yet.
//
// Coordinate convention: glTF is right-handed Y-up; this engine is Z-up. A pure
// Y-up -> Z-up rotation (x,y,z)->(x,-z,y) is applied to positions and normals.
// No scale is baked in — size models via the level's per-actor "scale" field.
namespace GltfImporter
{
    // Fills 'mesh' (vertex buffer + material) from the given file.
    // Returns true on success. assetManager owns any created textures/materials.
    bool Load(Mesh* mesh, const char* fileName, AssetManager* assetManager);
}
