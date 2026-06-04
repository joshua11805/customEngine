#pragma once
#include <string>
#include <vector>
#include <SDL3/SDL.h>

class Renderer;
class Shader;
class UIRenderer;
class UIWidget;

// Root of the UI hierarchy. Owned by Game.
//
// Frame contract (called from Game each frame):
//   1. canvas->Update(dt)           — game logic updates widget state
//   2. canvas->Prepare()            — build CPU geometry + upload to GPU
//                                     (must happen BEFORE BeginCommandBuffer)
//   3. canvas->Render(cmd, pass)    — issue draw calls inside a render pass
class UICanvas {
public:
    UICanvas()  = default;
    ~UICanvas() = default;

    bool Init(Renderer* renderer, int screenW, int screenH);
    void Shutdown();
    void OnResize(int w, int h) { m_screenW = w; m_screenH = h; }

    // Update all widget trees.
    void Update(float deltaTime);

    // Build and upload geometry. Call before BeginCommandBuffer.
    void Prepare();

    // Issue draw calls. shader must be the "UI" shader from AssetManager.
    void Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass, Shader* shader);

    // Add a root-level widget. Canvas takes ownership.
    void AddWidget(UIWidget* widget);

    UIRenderer* GetUIRenderer() const { return m_uiRenderer; }
    int GetScreenW() const { return m_screenW; }
    int GetScreenH() const { return m_screenH; }

private:
    UIRenderer*            m_uiRenderer = nullptr;
    Renderer*              m_renderer   = nullptr;
    std::vector<UIWidget*> m_widgets;
    int m_screenW = 800;
    int m_screenH = 600;
};
