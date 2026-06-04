#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H
#include "EngineMath.h"
#include "Material.h"
#include "Component.h"
class VertexBuffer;
class Mesh;
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
    virtual void Draw(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);
    PerObjectConstants GetWorldMat() { return m_POC; }
    void SetWorldMat(Matrix4 mat) { m_POC.c_modelToWorld = mat; }
    void SetTransform(Matrix4 mat) { m_POC.c_modelToWorld = mat; }
    Matrix4 GetTransform() const { return m_POC.c_modelToWorld; }
    Vector3 GetTransformVec() const { return m_POC.c_modelToWorld.GetTranslation(); }
    void AddComponent(Component* pComponent) { m_components.push_back(pComponent); }
    virtual void Update(float deltaTime);

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

protected:
    Mesh* m_mesh = nullptr;
    PerObjectConstants m_POC;
    std::string m_name;
private:
    std::vector<Component*> m_components;
};


#endif //GAME_ACTOR_H