#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H
#include "EngineMath.h"
#include "Material.h"
#include "Component.h"
class VertexBuffer;
class Mesh;
class Shader;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;


class Actor
{
public:
    struct PerObjectConstants
    {
        Matrix4 c_modelToWorld;
    };

    Actor(Mesh* mesh);
    virtual ~Actor();
    virtual void Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, Shader* shaderOverride = nullptr);

    // Override in subclasses that have a vertex format incompatible with the generic
    // scene-view shaders (e.g. SkinnedObj, TerrainChunk).  The render loop calls this
    // before passing sceneShader so each actor can substitute its own compatible pipeline.
    // mode matches SceneViewMode enum values; return nullptr to use the default sceneShader.
    virtual Shader* PickSceneShader(int /*mode*/) { return nullptr; }

    PerObjectConstants GetWorldMat() { return m_POC; }
    void SetWorldMat(Matrix4 mat) { m_POC.c_modelToWorld = mat; SyncDecomposed(); }
    void SetTransform(Matrix4 mat) { m_POC.c_modelToWorld = mat; SyncDecomposed(); }
    Matrix4 GetTransform() const { return m_POC.c_modelToWorld; }
    Vector3 GetTransformVec() const { return m_POC.c_modelToWorld.GetTranslation(); }
    void AddComponent(Component* pComponent) { m_components.push_back(pComponent); }
    virtual void Update(float deltaTime);

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    // Mesh source path — used for level serialization. Empty for runtime-only actors.
    void SetMeshPath(const std::string& path) { m_meshPath = path; }
    const std::string& GetMeshPath() const { return m_meshPath; }

    // Editor-friendly decomposed transform (Euler angles in degrees).
    // These are the authoritative values; the matrix is rebuilt from them.
    void  SetPosition(Vector3 pos);
    void  SetEulerDeg(Vector3 deg);   // yaw=Z, pitch=Y, roll=X (degrees)
    void  SetScale(Vector3 scale);
    Vector3 GetPosition()  const { return m_position; }
    Vector3 GetEulerDeg()  const { return m_eulerDeg; }
    Vector3 GetScale()     const { return m_scale; }

protected:
    Mesh* m_mesh = nullptr;
    PerObjectConstants m_POC;
    std::string m_name;

    // Decomposed transform — kept in sync with m_POC.c_modelToWorld
    Vector3 m_position  = Vector3::Zero;
    Vector3 m_eulerDeg  = Vector3::Zero;   // degrees: x=roll, y=pitch, z=yaw
    Vector3 m_scale     = Vector3(1.f, 1.f, 1.f);
    std::string m_meshPath; // relative path used when loading; set for serialization

    void RebuildMatrix();
    void SyncDecomposed(); // extracts position/scale from current matrix (rotation stays at zero)
private:
    std::vector<Component*> m_components;
};


#endif //GAME_ACTOR_H