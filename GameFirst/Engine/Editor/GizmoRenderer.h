#pragma once
#include "../EngineMath.h"
#include <vector>
#include <SDL3/SDL.h>

class Renderer;
class Shader;
class Texture;

// Vertex layout used by GizmoRenderer: world-space position + RGBA color
struct GizmoVertex
{
    Vector3 pos;
    float   r, g, b, a;
};

// GizmoMode: what transform tool is currently active in the Scene View
enum class GizmoMode { Translate, Rotate, Scale };

// Draws editor gizmos (translate/rotate/scale handles + selection outline) each frame.
// Usage each frame:
//   1. BeginFrame()             — clear line list
//   2. AddTranslateGizmo(...)   — or AddRotateGizmo / AddScaleGizmo
//   3. AddSelectionBox(...)     — AABB outline around selected actor
//   4. Upload(renderer)         — build GPU buffers (call BEFORE BeginCommandBuffer)
//   5. Draw(cmd, pass, camera)  — render into the scene editor pass
class GizmoRenderer
{
public:
    bool Init(Renderer* renderer, Texture* sceneRT);
    void Shutdown();

    // Begin a new frame — clears accumulated geometry
    void BeginFrame();

    // Add gizmo geometry for a selected actor.
    // transform: the actor's model-to-world matrix
    // handleSize: world-space length of the axis arrows
    void AddTranslateGizmo(const Matrix4& transform, float handleSize = 60.f);
    void AddRotateGizmo   (const Matrix4& transform, float handleSize = 60.f);
    void AddScaleGizmo    (const Matrix4& transform, float handleSize = 60.f);

    // Draws a wire AABB selection outline around the actor using its position + scale.
    // extents: half-size of the AABB in object space (defaults to 50 units, matching default cube/sphere)
    void AddSelectionBox(const Matrix4& transform, Vector3 extents = Vector3(50.f, 50.f, 50.f));

    // Upload accumulated geometry to the GPU (call before BeginCommandBuffer each frame)
    void Upload(Renderer* renderer);

    // Issue draw calls inside an active render pass.
    // Must call Camera::SetActive(cmd) before calling this so the VP matrix is pushed.
    void Draw(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass);

    bool HasGeometry() const { return !m_vertices.empty(); }

    // Hit-test a ray against a translate gizmo for dragging.
    // Returns 0=none, 1=X, 2=Y, 3=Z axis hit
    int HitTestTranslateGizmo(const Matrix4& transform, float handleSize,
                               Vector3 rayOrigin, Vector3 rayDir) const;

private:
    void AddLine(Vector3 a, Vector3 b, float r, float g, float b_, float a_);
    void AddArrow(Vector3 origin, Vector3 dir, float len, float r, float g, float b_);
    void AddCircle(Vector3 center, Vector3 normal, float radius, float r, float g, float b_, int segments = 32);

    std::vector<GizmoVertex> m_vertices;
    std::vector<uint16_t>    m_indices;

    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer  = nullptr;
    uint32_t       m_allocatedVerts  = 0;
    uint32_t       m_allocatedInds   = 0;

    Shader*  m_shader  = nullptr;
};
