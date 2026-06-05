#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <filesystem>
#include "../EngineMath.h"

class Renderer;
class Actor;
class Texture;

// Draw mode for the Scene View panel.
enum class SceneViewMode
{
    Lit,              // Phong shading — normal game appearance
    Unlit,            // Diffuse texture only, no lighting
    Wireframe,        // LINE fill, no shading
    WireframeOnShaded,// Lit pass + wireframe overlay in a second sub-pass
    LightContrib,     // Diffuse lighting accumulation only (no texture)
    VertexColor,      // Raw vertex COLOR0 channel (needs meshes with vertex colours)
};

// Per-frame data pushed from Game into the editor each frame.
// This avoids a circular Engine -> Game header dependency.
struct EditorFrameData
{
    const std::vector<Actor*>* actors         = nullptr;  // scene actor list (read-only)
    Texture*                   sceneEditorRT  = nullptr;  // scene view render target (editor cam, no post-fx)
    Texture*                   gameRT         = nullptr;  // game view render target (full pipeline)
    bool                       isPlaying      = false;
    int*                       selectedActor  = nullptr;  // pointer to Game's selection index
    // Output: editor writes the scene-camera world-to-view matrix here each frame.
    Matrix4*                   outSceneCamera    = nullptr;
    // Input: Game fills in the current scene view-projection matrix for raycasting.
    Matrix4                    sceneViewProj     = Matrix4::Identity;
    // I/O: editor reads + writes the current scene draw mode each frame.
    SceneViewMode*             sceneViewMode     = nullptr;
};

// Callback so the editor can request state changes in Game without including Game.h
struct EditorCallbacks
{
    void* userData = nullptr;
    void (*setPlaying)(void* ud, bool playing)                      = nullptr;
    void (*spawnPrimitive)(void* ud, int type, Vector3 worldPos)    = nullptr; // type: 0=cube, 1=sphere
    void (*spawnMesh)(void* ud, const char* path, Vector3 worldPos) = nullptr;
    void (*removeActor)(void* ud, int index)                        = nullptr;
    void (*saveLevel)(void* ud)                                     = nullptr;
};

// Owns the Dear ImGui lifetime and all editor panels.
//
// Frame contract (called from Game each frame):
//   1. ProcessEvent(event)          — in SDL_AppEvent, for every SDL event
//   2. BeginFrame(data, callbacks)  — in UpdateGame: starts ImGui frame + builds panels
//   3. EndFrame()                   — in UpdateGame: calls ImGui::Render() to finalize draw data
//   4. Prepare(cmd)                 — in RenderFrame after BeginCommandBuffer: uploads VB/IB
//   5. Render(cmd, pass)            — in RenderFrame: issues ImGui draw calls (Pass 10)
class EditorLayer {
public:
    bool Init(Renderer* renderer, const char* projectRoot = nullptr);
    void Shutdown();

    void ProcessEvent(SDL_Event* event);
    // Called when the OS delivers a file drop (SDL_EVENT_DROP_FILE).
    void NotifyOsFileDrop(const char* path);
    void BeginFrame(const EditorFrameData& data, const EditorCallbacks& callbacks);
    void EndFrame();
    void Prepare(SDL_GPUCommandBuffer* cmd);
    void Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass);

    bool WantCaptureMouse()    const;
    bool WantCaptureKeyboard() const;

private:
    void DrawMainMenuBar();
    void DrawSceneView();
    void DrawGameView();
    void DrawHierarchy();
    void DrawInspector();
    void DrawProjectView();

    // Project view helpers
    void DrawDirectoryNode(const std::filesystem::path& path);
    void OpenInIDE(const std::filesystem::path& path);
    void CreateScript(const std::string& name, const std::filesystem::path& dir);

    // Copied each frame from EditorFrameData
    const std::vector<Actor*>* m_actors         = nullptr;
    Texture*                   m_sceneEditorRT  = nullptr;
    Texture*                   m_gameRT         = nullptr;
    bool                       m_isPlaying      = false;
    int*                       m_selectedActor  = nullptr;
    Matrix4*                   m_outSceneCamera = nullptr;
    SceneViewMode*             m_sceneViewMode  = nullptr;
    EditorCallbacks            m_callbacks      = {};

    // Panel visibility toggles
    bool m_showHierarchy    = true;
    bool m_showSceneView    = true;
    bool m_showGameView     = true;
    bool m_showInspector    = true;
    bool m_showProjectView  = true;

    // Project view state
    std::filesystem::path m_projectRoot;       // set from Game via Init
    char  m_newScriptName[128] = {};
    bool  m_newScriptPopupOpen = false;
    std::filesystem::path m_newScriptTargetDir;

    // True after the first DockBuilder layout has been applied
    bool m_layoutInitialized = false;

    // Asset drag-and-drop: path set by Project View, consumed by Scene View drop target
    std::string m_draggedAssetPath;

    // OS file drop into Project panel
    std::string                m_osPendingDropPath; // set from SDL_EVENT_DROP_FILE
    std::filesystem::path      m_projectHoveredDir; // last dir hovered in DrawDirectoryNode

    // Scene-view camera state (editor-only, independent of game camera)
    Vector3 m_scenePos   = Vector3(500.f, 0.f, 100.f); // world position
    float   m_sceneYaw   = Math::Pi;   // horizontal angle (radians)
    float   m_scenePitch = -0.2f;      // vertical angle (radians, clamped)
    float   m_scenePivotDist = 300.f;  // orbit pivot distance

    void UpdateSceneCamera(bool panelHovered);
    Matrix4 BuildSceneCameraMatrix() const;

    // Screen-to-world: unprojects a Scene View pixel to a world position on Z=groundZ plane.
    // All pixel/panel values are screen-space floats. Returns false if ray misses the plane.
    bool ScreenToWorldGround(float pixelX, float pixelY,
                             float panelX, float panelY, float panelW, float panelH,
                             float groundZ, Vector3& outWorld) const;

    // Cached per-frame values set during DrawSceneView (used by toolbar buttons)
    float   m_sceneViewX    = 0.f;
    float   m_sceneViewY    = 0.f;
    float   m_sceneViewW    = 0.f;
    float   m_sceneViewH    = 0.f;
    Matrix4 m_sceneViewProj = Matrix4::Identity; // copied from EditorFrameData each frame
};
