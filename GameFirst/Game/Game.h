#pragma once
#include "Renderer.h"
#include "EngineMath.h"
#include "../Engine/TestCube.h"
#include "../Engine/UI/UICanvas.h"
#include "../Engine/Editor/EditorLayer.h"  // for SceneViewMode
#include <vector>

class Actor;
class Texture;

class Game {
public:
    Game();
    ~Game();

    bool Init(int width, int height);
    void Shutdown();
    void ProcessInput();
    void UpdateGame();
    void RenderFrame();
    void OnResize(int w, int h);
    void ProcessEditorEvent(SDL_Event* event);

    // Polling Input Status
    bool IsKeyHeld(int key) const;
    bool IsKeyJustPressed(int key) const;
    Vector2 GetRelativeMouse() const;
    Vector2 GetRawRelativeMouse() const;
    uint32_t GetMouseButtons() const;
    bool IsMouseButtonJustPressed(uint32_t buttonMask) const;
    void SetMouseCapture(bool capture);
    class Camera* GetCamera() { return m_camera; }

    // Editor API
    void EditorSaveLevel() { SaveLevel(m_currentLevelPath.c_str()); }
    const std::vector<Actor*>& GetActors() const { return m_actors; }
    class Texture* GetSceneEditorTarget() const { return m_sceneEditorTarget; }
    class Texture* GetGameTarget()        const { return m_gameTarget; }
    bool IsPlaying() const { return m_isPlaying; }
    void SetPlaying(bool playing);
    int GetSelectedActorIndex() const { return m_selectedActor; }
    void SetSelectedActorIndex(int idx) { m_selectedActor = idx; }

    // Spawn / remove actors at runtime (called by EditorCallbacks)
    void EditorSpawnPrimitive(int type, Vector3 worldPos); // 0=cube, 1=sphere
    void EditorSpawnMesh(const char* path, Vector3 worldPos);
    void EditorRemoveActor(int index);
    void EditorSpawnTerrain();

    class TerrainManager* GetTerrainManager() const { return m_terrainManager; }
    class TerrainActor*   GetTerrainActor()   const { return m_terrainActor; }

private:
    Renderer m_renderer;
    uint64_t m_ticksCount = 0;
    bool m_isFullscreen = false;
    bool m_isPlaying = false;
    int m_selectedActor = -1;
    Matrix4 m_sceneCameraMatrix; // written by EditorLayer each frame, used by RenderFrame when not playing
    SceneViewMode m_sceneViewMode = SceneViewMode::Lit;

    // Input
    const bool* m_keyState = nullptr;
    bool* m_lastKeyState = nullptr;
    int m_numKeys = 0;
    uint32_t m_mouseButtons = 0;
    uint32_t m_lastMouseButtons = 0;
    Vector2 m_relativeMouse = Vector2::Zero;

    //asset manager
    class AssetManager* m_assetManager = nullptr;

    //Actors
    std::vector<Actor*> m_actors;
    std::string m_currentLevelPath; // path last passed to LoadLevel()

    Camera* m_camera       = nullptr;  // game / play-mode camera
    Camera* m_editorCamera = nullptr;  // scene-view camera (editor only)
    Mesh* m_cubeMesh = nullptr;
    class Lighting* m_lighting = nullptr;
    class Physics* m_physics = nullptr;
    VertexBuffer* m_vertexBuffer = nullptr;

    // UI
    UICanvas*  m_uiCanvas = nullptr;
    class Font*    m_uiFont   = nullptr;
    class UIText*  m_fpsText  = nullptr;  // non-owning; UICanvas owns it

    // Editor
    class EditorLayer* m_editorLayer = nullptr;

    // Terrain
    class TerrainManager* m_terrainManager = nullptr; // preview only (scene view streaming)
    class TerrainActor*   m_terrainActor   = nullptr; // non-owning; owned by m_actors when spawned

    // Render targets
    Texture* m_sceneEditorTarget = nullptr; // scene view (editor camera, no post-fx)
    Texture* m_gameTarget        = nullptr; // game view final output (full pipeline)
    Texture* m_renderTarget      = nullptr; // game pipeline intermediate (pre-bloom)
    Texture* m_halfTarget        = nullptr;
    Texture* m_qTarget1          = nullptr;
    Texture* m_qTarget2          = nullptr;

    //temp struct for blur
    struct BlurConstants
    {
        Vector2 c_blurDir;
        Vector2 c_texelSize;
    };

    struct CrosshairConstants
    {
        Vector2 c_screenSize;
        float _pad[2];
    };



    void LoadShaders();
    bool LoadLevel(const char* fileName);
    bool SaveLevel(const char* fileName);
};
