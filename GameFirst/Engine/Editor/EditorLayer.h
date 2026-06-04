#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include "../EngineMath.h"

class Renderer;
class Actor;
class Texture;

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
    Matrix4*                   outSceneCamera = nullptr;
};

// Callback so the editor can request state changes in Game without including Game.h
struct EditorCallbacks
{
    void* userData = nullptr;
    void (*setPlaying)(void* ud, bool playing) = nullptr;
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
    bool Init(Renderer* renderer);
    void Shutdown();

    void ProcessEvent(SDL_Event* event);
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

    // Copied each frame from EditorFrameData
    const std::vector<Actor*>* m_actors         = nullptr;
    Texture*                   m_sceneEditorRT  = nullptr;
    Texture*                   m_gameRT         = nullptr;
    bool                       m_isPlaying      = false;
    int*                       m_selectedActor  = nullptr;
    Matrix4*                   m_outSceneCamera = nullptr;
    EditorCallbacks            m_callbacks      = {};

    // Panel visibility toggles
    bool m_showHierarchy  = true;
    bool m_showSceneView  = true;
    bool m_showGameView   = true;
    bool m_showInspector  = true;

    // True after the first DockBuilder layout has been applied
    bool m_layoutInitialized = false;

    // Scene-view camera state (editor-only, independent of game camera)
    Vector3 m_scenePos   = Vector3(500.f, 0.f, 100.f); // world position
    float   m_sceneYaw   = Math::Pi;   // horizontal angle (radians)
    float   m_scenePitch = -0.2f;      // vertical angle (radians, clamped)
    float   m_scenePivotDist = 300.f;  // orbit pivot distance

    void UpdateSceneCamera(bool panelHovered);
    Matrix4 BuildSceneCameraMatrix() const;
};
