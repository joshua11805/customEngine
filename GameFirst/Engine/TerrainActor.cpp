#include "pch.h"
#include "TerrainActor.h"
#include "TerrainManager.h"
#include "Shader.h"

TerrainActor::TerrainActor()
    : Actor(nullptr)
{
    m_manager = new TerrainManager();
    SetName("Terrain");
}

TerrainActor::~TerrainActor()
{
    delete m_manager;
    m_manager = nullptr;
}

void TerrainActor::UpdateStreaming(const Vector3& cameraPos)
{
    if (m_manager)
        m_manager->Update(cameraPos - GetPosition());
}

void TerrainActor::Draw(SDL_GPUCommandBuffer* commandBuffer,
                        SDL_GPURenderPass*   renderPass,
                        Shader*              shaderOverride)
{
    if (!m_manager) return;
    if (!shaderOverride && TerrainChunk::IsGamePass() && m_gameShader)
        shaderOverride = m_gameShader;
    m_manager->Draw(commandBuffer, renderPass, shaderOverride, GetPosition());
}

Shader* TerrainActor::PickSceneShader(int mode)
{
    // SceneViewMode: 2=Wireframe, 3=WireframeOnShaded overlay sub-pass
    if ((mode == 2 || mode == 3) && m_wireShader)
        return m_wireShader;
    // All other modes: return the terrain scene Lit shader so the render loop never
    // substitutes the generic SceneLit pipeline (VertexPosNormalUV — wrong format).
    return m_sceneShader;
}

void TerrainActor::SetShaders(Shader* scene, Shader* game, Shader* sceneWire)
{
    m_sceneShader = scene;
    m_gameShader  = game;
    m_wireShader  = sceneWire;
    if (m_manager) m_manager->SetShaders(scene, game, sceneWire);
}

const TerrainChunk::GenParams& TerrainActor::GetParams() const
{
    return m_manager->GetParams();
}

void TerrainActor::SetParams(const TerrainChunk::GenParams& p)
{
    m_manager->SetParams(p);
}
