#pragma once
#include <SDL3/SDL.h>

class Renderer;

// Owns the Dear ImGui lifetime for the editor overlay.
//
// Frame contract (called from Game each frame):
//   1. ProcessEvent(event)    — in SDL_AppEvent, for every SDL event
//   2. BeginFrame()           — in UpdateGame: starts ImGui frame + builds panels
//   3. EndFrame()             — in UpdateGame: calls ImGui::Render() to finalize draw data
//   4. Prepare(cmd)           — in RenderFrame after BeginCommandBuffer: uploads VB/IB
//   5. Render(cmd, pass)      — in RenderFrame: issues ImGui draw calls (Pass 10)
class EditorLayer {
public:
    bool Init(Renderer* renderer);
    void Shutdown();

    void ProcessEvent(SDL_Event* event);
    void BeginFrame();
    void EndFrame();
    void Prepare(SDL_GPUCommandBuffer* cmd);
    void Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass);

    bool WantCaptureMouse()    const;
    bool WantCaptureKeyboard() const;
};
