#pragma once
#include "Renderer.h"
#include "EngineMath.h"
#include "../Engine/TestCube.h"

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

    // Polling Input Status
    bool IsKeyHeld(int key) const;
    bool IsKeyJustPressed(int key) const;
    Vector2 GetRelativeMouse() const;
    Vector2 GetRawRelativeMouse() const;
    uint32_t GetMouseButtons() const;
    bool IsMouseButtonJustPressed(uint32_t buttonMask) const;
    void SetMouseCapture(bool capture);
    class Camera* GetCamera() { return m_camera; }

private:
    Renderer m_renderer;
    uint64_t m_ticksCount = 0;
    bool m_isFullscreen = false;

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

    Camera* m_camera = nullptr;
    Mesh* m_cubeMesh = nullptr;
    class Lighting* m_lighting = nullptr;
    class Physics* m_physics = nullptr;
    VertexBuffer* m_vertexBuffer = nullptr;

    //render target
    Texture* m_renderTarget = nullptr;
    Texture* m_halfTarget = nullptr;
    Texture* m_qTarget1 = nullptr;
    Texture* m_qTarget2 = nullptr;

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
};
