#include "pch.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "GltfImporter.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "AssetManager.h"
#include "VertexFormat.h"
#include "Renderer.h"

#include <cmath>

namespace
{
    // glTF right-handed Y-up  ->  engine Z-up: (x,y,z) -> (x,-z,y)
    inline Vector3 YUpToZUp(float x, float y, float z) { return Vector3(x, -z, y); }

    // Directory portion of a path, including trailing slash.
    std::string DirectoryOf(const char* path)
    {
        std::string p(path);
        size_t s = p.find_last_of("/\\");
        return (s == std::string::npos) ? std::string() : p.substr(0, s + 1);
    }

    // Load/cache a texture from a cgltf image.
    Texture* LoadGltfImage(const cgltf_image* image, const char* gltfFile,
                           const std::string& cacheKey, AssetManager* assetManager)
    {
        if (!image) return nullptr;
        Texture* tex = new Texture();
        if (image->buffer_view)
        {
            const cgltf_buffer_view* bv = image->buffer_view;
            const unsigned char* bytes = static_cast<const unsigned char*>(bv->buffer->data) + bv->offset;
            if (!tex->LoadFromMemory(bytes, static_cast<int>(bv->size), cacheKey.c_str()))
            { delete tex; return nullptr; }
        }
        else if (image->uri && SDL_strncmp(image->uri, "data:", 5) != 0)
        {
            std::string path = DirectoryOf(gltfFile) + image->uri;
            if (!tex->Load(path.c_str()))
            { delete tex; return nullptr; }
        }
        else
        {
            SDL_Log("glTF import: unsupported image source (data URI) in %s", gltfFile);
            delete tex; return nullptr;
        }
        assetManager->CacheTexture(cacheKey, tex);
        return tex;
    }

    // Build a Phong Material from a glTF PBR material entry.
    Material* BuildMaterial(const cgltf_material* gltfMat, const char* gltfFile,
                            const std::string& matKey, const std::string& texKey,
                            AssetManager* assetManager)
    {
        Material* material = new Material();
        material->SetShader(assetManager->GetShader("Phong"));

        Vector3 baseColor(1.0f, 1.0f, 1.0f);
        const cgltf_image* baseColorImage = nullptr;

        if (gltfMat && gltfMat->has_pbr_metallic_roughness)
        {
            const cgltf_pbr_metallic_roughness& pbr = gltfMat->pbr_metallic_roughness;
            baseColor = Vector3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);
            if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
                baseColorImage = pbr.base_color_texture.texture->image;
        }

        material->SetDiffuseColor(baseColor);
        material->SetSpecularColor(Vector3(1.0f, 1.0f, 1.0f));
        material->SetSpecularPower(16.0f);

        // The Phong shader always samples texture slot 0 — we must bind something.
        Texture* diffuse = LoadGltfImage(baseColorImage, gltfFile, texKey, assetManager);
        if (!diffuse)
        {
            // No texture: solid 1×1 tinted by baseColorFactor so the model shows its color.
            Texture* solid = new Texture();
            solid->CreateSolidColor(
                static_cast<uint8_t>(baseColor.x * 255.0f),
                static_cast<uint8_t>(baseColor.y * 255.0f),
                static_cast<uint8_t>(baseColor.z * 255.0f),
                255, texKey.c_str());
            assetManager->CacheTexture(texKey, solid);
            diffuse = solid;
        }
        material->SetTexture(Renderer::TEXTURE_SLOT_DIFFUSE, diffuse);
        assetManager->CacheMaterial(matKey, material);
        return material;
    }

    // One bucket of geometry per glTF material pointer.
    struct MaterialGroup
    {
        const cgltf_material* gltfMat = nullptr;
        std::vector<VertexPosNormalUV> vertices;
        std::vector<uint32_t> indices;
    };

    // Apply column-major 4×4 matrix to a point (w=1).
    inline void TransformPoint(const cgltf_float m[16], float x, float y, float z,
                               float& ox, float& oy, float& oz)
    {
        ox = m[0]*x + m[4]*y + m[8]*z  + m[12];
        oy = m[1]*x + m[5]*y + m[9]*z  + m[13];
        oz = m[2]*x + m[6]*y + m[10]*z + m[14];
    }

    // Apply column-major 3×3 (upper-left) to a direction (w=0), then normalise.
    inline void TransformNormal(const cgltf_float m[16], float x, float y, float z,
                                float& ox, float& oy, float& oz)
    {
        ox = m[0]*x + m[4]*y + m[8]*z;
        oy = m[1]*x + m[5]*y + m[9]*z;
        oz = m[2]*x + m[6]*y + m[10]*z;
        float len = sqrtf(ox*ox + oy*oy + oz*oz);
        if (len > 1e-6f) { ox /= len; oy /= len; oz /= len; }
    }

    // Recursive node traversal — fills material groups with world-space vertices.
    void ProcessNode(const cgltf_node* node, std::vector<MaterialGroup>& groups)
    {
        cgltf_float wm[16];
        cgltf_node_transform_world(node, wm);

        if (node->mesh)
        {
            for (size_t pi = 0; pi < node->mesh->primitives_count; ++pi)
            {
                const cgltf_primitive& prim = node->mesh->primitives[pi];
                if (prim.type != cgltf_primitive_type_triangles) continue;

                const cgltf_accessor *posAcc = nullptr, *normAcc = nullptr, *uvAcc = nullptr;
                for (size_t a = 0; a < prim.attributes_count; ++a)
                {
                    const cgltf_attribute& attr = prim.attributes[a];
                    if      (attr.type == cgltf_attribute_type_position)                       posAcc  = attr.data;
                    else if (attr.type == cgltf_attribute_type_normal)                         normAcc = attr.data;
                    else if (attr.type == cgltf_attribute_type_texcoord && attr.index == 0)    uvAcc   = attr.data;
                }
                if (!posAcc) continue;

                // Find or create the bucket for this material.
                MaterialGroup* group = nullptr;
                for (auto& g : groups) if (g.gltfMat == prim.material) { group = &g; break; }
                if (!group) { groups.push_back({prim.material, {}, {}}); group = &groups.back(); }

                const uint32_t base = static_cast<uint32_t>(group->vertices.size());
                for (size_t v = 0; v < posAcc->count; ++v)
                {
                    float p[3] = {0,0,0}, n[3] = {0,0,1}, uv[2] = {0,0};
                    cgltf_accessor_read_float(posAcc, v, p, 3);
                    if (normAcc) cgltf_accessor_read_float(normAcc, v, n, 3);
                    if (uvAcc)   cgltf_accessor_read_float(uvAcc,   v, uv, 2);

                    // 1. Transform to glTF world space (Y-up) via node world matrix.
                    float wp[3], wn[3];
                    TransformPoint (wm, p[0], p[1], p[2], wp[0], wp[1], wp[2]);
                    TransformNormal(wm, n[0], n[1], n[2], wn[0], wn[1], wn[2]);

                    // 2. Convert glTF Y-up world space to engine Z-up space.
                    VertexPosNormalUV vert;
                    vert.pos    = YUpToZUp(wp[0], wp[1], wp[2]);
                    vert.normal = YUpToZUp(wn[0], wn[1], wn[2]);
                    vert.uv     = Vector2(uv[0], uv[1]);
                    group->vertices.push_back(vert);
                }

                // Swap i1/i2 per triangle to fix winding: Y-up->Z-up flips handedness,
                // turning CCW (glTF front) into CW — reversing two indices corrects it.
                if (prim.indices)
                {
                    for (size_t ii = 0; ii + 2 < prim.indices->count; ii += 3)
                    {
                        group->indices.push_back(base + static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii)));
                        group->indices.push_back(base + static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii + 2)));
                        group->indices.push_back(base + static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii + 1)));
                    }
                }
                else
                {
                    for (uint32_t ii = 0; ii + 2 < static_cast<uint32_t>(posAcc->count); ii += 3)
                    {
                        group->indices.push_back(base + ii);
                        group->indices.push_back(base + ii + 2);
                        group->indices.push_back(base + ii + 1);
                    }
                }
            }
        }

        for (size_t ci = 0; ci < node->children_count; ++ci)
            ProcessNode(node->children[ci], groups);
    }
}

bool GltfImporter::Load(Mesh* mesh, const char* fileName, AssetManager* assetManager)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, fileName, &data) != cgltf_result_success)
    {
        SDL_Log("glTF import: failed to parse %s", fileName);
        return false;
    }
    if (cgltf_load_buffers(&options, data, fileName) != cgltf_result_success)
    {
        SDL_Log("glTF import: failed to load buffers for %s", fileName);
        cgltf_free(data);
        return false;
    }

    // Traverse the default scene (or all root nodes if no scene is specified).
    std::vector<MaterialGroup> groups;
    const cgltf_scene* scene = data->scene
        ? data->scene
        : (data->scenes_count ? &data->scenes[0] : nullptr);

    if (scene)
    {
        for (size_t ni = 0; ni < scene->nodes_count; ++ni)
            ProcessNode(scene->nodes[ni], groups);
    }
    else
    {
        for (size_t ni = 0; ni < data->nodes_count; ++ni)
            if (!data->nodes[ni].parent)
                ProcessNode(&data->nodes[ni], groups);
    }

    if (groups.empty())
    {
        SDL_Log("glTF import: no triangle geometry found in %s", fileName);
        cgltf_free(data);
        return false;
    }

    SDL_Log("glTF import: %s - %u material group(s)", fileName, (unsigned)groups.size());

    // Build one sub-mesh per material group and attach to the root mesh.
    int added = 0;
    for (size_t gi = 0; gi < groups.size(); ++gi)
    {
        const MaterialGroup& g = groups[gi];
        if (g.vertices.empty()) continue;

        std::string matKey = std::string(fileName) + "#mat" + std::to_string(gi);
        std::string texKey = std::string(fileName) + "#tex" + std::to_string(gi);
        Material* mat = BuildMaterial(g.gltfMat, fileName, matKey, texKey, assetManager);

        Mesh* sub = new Mesh(nullptr, nullptr);
        bool ok;
        if (g.vertices.size() > 0xFFFF)
        {
            ok = sub->Load(
                const_cast<VertexPosNormalUV*>(g.vertices.data()),
                static_cast<uint32_t>(g.vertices.size() * sizeof(VertexPosNormalUV)),
                const_cast<uint32_t*>(g.indices.data()),
                static_cast<uint32_t>(g.indices.size()), sizeof(uint32_t), mat);
        }
        else
        {
            std::vector<uint16_t> idx16(g.indices.begin(), g.indices.end());
            ok = sub->Load(
                const_cast<VertexPosNormalUV*>(g.vertices.data()),
                static_cast<uint32_t>(g.vertices.size() * sizeof(VertexPosNormalUV)),
                idx16.data(),
                static_cast<uint32_t>(idx16.size()), sizeof(uint16_t), mat);
        }

        if (ok) { mesh->AddSubMesh(sub); ++added; }
        else    { delete sub; }
    }

    cgltf_free(data);
    return added > 0;
}
