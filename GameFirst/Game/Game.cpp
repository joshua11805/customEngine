#include "pch.h"
#include "Game.h"
#include "../Engine/UI/UICanvas.h"
#include "../Engine/Editor/EditorLayer.h"
#include "../Engine/UI/UIPanel.h"
#include "../Engine/UI/UIText.h"
#include "../Engine/UI/Font.h"
#include "JsonUtil.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/ostreamwrapper.h"
#include "Mesh.h"
#include "Profiler.h"
#include "Shader.h"
#include "VertexFormat.h"
#include "Camera.h"
#include "Actor.h"
#include "AssetManager.h"
#include "Lighting.h"
#include "Material.h"
#include "../Engine/PointLight.h"
#include "JobManager.h"
#include "Physics.h"
#include "../Engine/Character.h"
#include "SkinnedObj.h"
#include "../Engine/CollisionBox.h"
#include "Components/Player.h"
#include "Components/FollowCam.h"
#include "Components/FPSCamera.h"
#include "Components/TargetMover.h"
#include "SimpleRotate.h"
#include "VertexBuffer.h"
#include "../Engine/PrimitiveMesh.h"
#include "../Engine/TerrainManager.h"
#include "../Engine/TerrainActor.h"
#include <filesystem>
float rotAng = 1.0f;

Game::Game()
{
}

Game::~Game()
{
}

/// One-time Initialization of the Game.
/// @param width the width of the window in pixels
/// @param height the height of the window in pixels
/// @return true on success or false on failure
bool Game::Init(int width, int height)
{
    PROFILE_SCOPE(GameInit);
    if (false == m_renderer.Init(800, 600))
    {
        DbgAssert(false, "Failed to init renderer");
        return false;
    }

    m_assetManager = new AssetManager();

    int w = m_renderer.GetScreenWidth();
    int h = m_renderer.GetScreenHeight();

    // Scene editor target — raw scene, editor camera, no post-fx
    m_sceneEditorTarget = new Texture();
    m_sceneEditorTarget->CreateRenderTarget(w, h, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "SceneEditorTarget");

    // Game pipeline targets
    m_gameTarget = new Texture();
    m_gameTarget->CreateRenderTarget(w, h, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, "GameTarget");

    m_renderTarget = new Texture();
    m_renderTarget->CreateRenderTarget(w, h, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "GameIntermediateTarget");

    m_halfTarget = new Texture();
    m_halfTarget->CreateRenderTarget(w/2, h/2, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "HalfSizeTarget");

    m_qTarget1 = new Texture();
    m_qTarget1->CreateRenderTarget(w/4, h/4, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "QuarterTarget1");

    m_qTarget2 = new Texture();
    m_qTarget2->CreateRenderTarget(w/4, h/4, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "QuarterTarget2");

    LoadShaders();

    {
        //create the game camera (play mode)
        m_camera = new Camera(&m_renderer);
        //create the editor camera (scene view)
        m_editorCamera = new Camera(&m_renderer);
        //create the lighting
        m_lighting = new Lighting();
        //create physics before load level
        m_physics = new Physics();
        Physics::SetInstance(m_physics);

        //being job manager
        JobManager::Get()->Begin();

        LoadLevel("Assets/Levels/Level_AimTrainer.itplevel");

        //create vertex buffer
        VertexPosUV quadVerts[] = {
            { Vector3(-1.0f, -1.0f, 0.0f), Vector2(0.0f, 1.0f) },
            { Vector3( 1.0f, -1.0f, 0.0f), Vector2(1.0f, 1.0f) },
            { Vector3( 1.0f,  1.0f, 0.0f), Vector2(1.0f, 0.0f) },
            { Vector3(-1.0f,  1.0f, 0.0f), Vector2(0.0f, 0.0f) }
        };
        uint16_t quadIndices[] = {
            0, 1, 2,
            0, 2, 3
        };
        m_vertexBuffer = new VertexBuffer(
            Renderer::Get(),
            quadVerts, sizeof(quadVerts),
            quadIndices, 6, sizeof(uint16_t)
        );

        Actor* cameraActor = new Actor(nullptr);
        cameraActor->SetName("Camera");
        cameraActor->SetWorldMat(Matrix4::CreateTranslation(Vector3(0, 0, 0)));
        FPSCamera* fps = new FPSCamera(cameraActor, this);
        cameraActor->AddComponent(fps);
        m_actors.push_back(cameraActor);
    }

    // UI canvas — must be initialized after LoadShaders so the "UI" shader is ready.
    m_uiCanvas = new UICanvas();
    if (!m_uiCanvas->Init(&m_renderer, m_renderer.GetScreenWidth(), m_renderer.GetScreenHeight()))
    {
        SDL_Log("Game::Init — UICanvas init failed");
        delete m_uiCanvas;
        m_uiCanvas = nullptr;
    }

    // FPS counter — top-left corner
    m_uiFont = new Font();
    if (m_uiCanvas && m_uiFont->Load("Assets/Fonts/consola.ttf", 20.f, &m_renderer))
    {
        m_fpsText = new UIText();
        m_fpsText->SetFont(m_uiFont);
        m_fpsText->SetText("FPS: --");
        m_fpsText->SetPosition(8.f, 8.f);
        m_fpsText->SetColor(Color4(0.f, 1.f, 0.f, 1.f));
        m_uiCanvas->AddWidget(m_fpsText);
    }
    else
    {
        SDL_Log("Game::Init — failed to load UI font; FPS counter disabled");
        delete m_uiFont;
        m_uiFont = nullptr;
    }

    // Terrain manager (disabled until the editor panel enables it)
    m_terrainManager = new TerrainManager();

    // Editor overlay
    m_editorLayer = new EditorLayer();
    if (!m_editorLayer->Init(&m_renderer, PROJECT_ROOT))
    {
        SDL_Log("Game::Init — EditorLayer init failed");
        delete m_editorLayer;
        m_editorLayer = nullptr;
    }

    // Default scene camera: matches the EditorLayer's initial position/yaw/pitch
    // so the first frame renders correctly before any input.
    {
        float yaw   = Math::Pi;
        float pitch = -0.2f;
        Vector3 pos(500.f, 0.f, 100.f);
        Matrix4 rot = Matrix4::CreateRotationY(pitch) * Matrix4::CreateRotationZ(yaw);
        Matrix4 cam = rot * Matrix4::CreateTranslation(pos);
        cam.Invert();
        m_sceneCameraMatrix = cam;
    }

    m_ticksCount = SDL_GetTicks();
    return true;
}

/// Shut down the game and release all resources
/// We do this in Shutdown BEFORE the destructor so that we can check for leaks
void Game::Shutdown()
{
    JobManager::Get()->End();

    if (m_editorLayer) { m_editorLayer->Shutdown(); delete m_editorLayer; m_editorLayer = nullptr; }

    delete m_terrainManager; m_terrainManager = nullptr;

    m_fpsText = nullptr; // owned by UICanvas
    if (m_uiCanvas) { m_uiCanvas->Shutdown(); delete m_uiCanvas; m_uiCanvas = nullptr; }
    delete m_uiFont; m_uiFont = nullptr;

    delete[] m_lastKeyState;
    delete[] m_keyState;

    for (Actor* a : m_actors)
    {
        delete a;
    }
    m_actors.clear();

    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;

    delete m_camera;
    m_camera = nullptr;
    delete m_editorCamera;
    m_editorCamera = nullptr;

    m_assetManager->Clear();
    delete m_assetManager;
    m_assetManager = nullptr;

    delete m_physics;
    m_physics = nullptr;

    delete m_sceneEditorTarget;
    m_sceneEditorTarget = nullptr;

    delete m_gameTarget;
    m_gameTarget = nullptr;

    delete m_renderTarget;
    m_renderTarget = nullptr;

    delete m_halfTarget;
    m_halfTarget = nullptr;

    delete m_qTarget2;
    m_qTarget2 = nullptr;

    delete m_qTarget1;
    m_qTarget1 = nullptr;

    m_renderer.Shutdown();
}

/// Process the input for a single frame of gameplay
void Game::ProcessInput()
{
    // copy the keys from last frame to m_lastKeyState
    if (m_numKeys > 0)
        memcpy((void*)m_lastKeyState, (void*)m_keyState, m_numKeys * sizeof(bool));
    m_lastMouseButtons = m_mouseButtons;

    // read the current keys
    int numKeys = 0;
    const bool* keyState = SDL_GetKeyboardState(&numKeys);

    if (numKeys > m_numKeys)
    {   // reallocate the array of keys (both current and last frame)
        if (m_keyState)
            delete[] m_keyState;
        if (m_lastKeyState)
            delete[] m_lastKeyState;
        m_keyState = new bool[numKeys]();
        m_lastKeyState = new bool[numKeys]();
    }

    // store the current frame's keys as m_keyState
    if (m_numKeys > 0)
        memcpy((void*)m_keyState, (void*)keyState, m_numKeys * sizeof(bool));
    m_numKeys = numKeys;

    // read the mouse
    m_mouseButtons = SDL_GetRelativeMouseState(&m_relativeMouse.x, &m_relativeMouse.y);

    // F11 toggles fullscreen
    if (IsKeyJustPressed(SDL_SCANCODE_F11))
    {
        m_isFullscreen = !m_isFullscreen;
        SDL_SetWindowFullscreen(m_renderer.GetWindow(), m_isFullscreen);
    }
}

/// Update a frame of gameplay
void Game::UpdateGame()
{
    PROFILE_SCOPE(UpdateGame);
    // calculate the length of this frame
    float deltaTime = 0.0f;
    {
        //I should probably expose these numbers so that it can readjust frame rate as needed.
        // Compute delta time
        uint64_t tickNow = SDL_GetTicks();

        // wait until 16 ms have elapsed at least
        while (tickNow - m_ticksCount < 8)
        {
            tickNow = SDL_GetTicks();
        }
        // Get deltaTime in seconds
        deltaTime = (tickNow - m_ticksCount) / 1000.0f;
        m_ticksCount = tickNow;
        // cap it to 33 ms
        if (deltaTime > 0.033f)
        {
            deltaTime = 0.033f;
        }

        // Update terrain streaming — preview manager (editor) and any spawned TerrainActor.
        // m_sceneCameraMatrix / returnWorldToCamera() are world-to-camera matrices;
        // invert to get camera-to-world, then read the translation as world position.
        {
            Matrix4 camToWorld = m_isPlaying
                ? m_camera->returnWorldToCamera()
                : m_sceneCameraMatrix;
            camToWorld.Invert();
            Vector3 camPos = camToWorld.GetTranslation();

            // Preview streaming (editor panel "Enable Terrain" checkbox)
            if (m_terrainManager)
                m_terrainManager->Update(camPos);

            // Placed terrain actors in the level stream independently
            for (Actor* a : m_actors)
            {
                if (TerrainActor* ta = dynamic_cast<TerrainActor*>(a))
                    ta->UpdateStreaming(camPos);
            }
        }

        if (m_isPlaying)
        {
            for (Actor* actor : m_actors)
                actor->Update(deltaTime);

            if (m_uiCanvas)
                m_uiCanvas->Update(deltaTime);
        }

        if (m_fpsText && deltaTime > 0.f)
        {
            char buf[32];
            SDL_snprintf(buf, sizeof(buf), "FPS: %.0f", 1.0f / deltaTime);
            m_fpsText->SetText(buf);
        }

        if (m_editorLayer)
        {
            // Compute VP for the editor camera so the editor can raycast
            m_editorCamera->SetWorldToCamera(m_sceneCameraMatrix);
            m_editorCamera->UpdateViewProj();

            EditorFrameData frameData;
            frameData.actors         = &m_actors;
            frameData.sceneEditorRT  = m_sceneEditorTarget;
            frameData.gameRT         = m_gameTarget;
            frameData.isPlaying      = m_isPlaying;
            frameData.selectedActor  = &m_selectedActor;
            frameData.outSceneCamera = &m_sceneCameraMatrix;
            frameData.sceneViewProj  = m_editorCamera->GetViewProj();
            frameData.sceneViewMode  = &m_sceneViewMode;
            frameData.terrainManager = m_terrainManager;
            frameData.terrainActor   = m_terrainActor;

            EditorCallbacks callbacks;
            callbacks.userData      = this;
            callbacks.setPlaying    = [](void* ud, bool playing) {
                static_cast<Game*>(ud)->SetPlaying(playing);
            };
            callbacks.spawnPrimitive = [](void* ud, int type, Vector3 worldPos) {
                static_cast<Game*>(ud)->EditorSpawnPrimitive(type, worldPos);
            };
            callbacks.spawnMesh = [](void* ud, const char* path, Vector3 worldPos) {
                static_cast<Game*>(ud)->EditorSpawnMesh(path, worldPos);
            };
            callbacks.removeActor = [](void* ud, int index) {
                static_cast<Game*>(ud)->EditorRemoveActor(index);
            };
            callbacks.saveLevel = [](void* ud) {
                static_cast<Game*>(ud)->EditorSaveLevel();
            };
            callbacks.spawnTerrain = [](void* ud) {
                static_cast<Game*>(ud)->EditorSpawnTerrain();
            };

            m_editorLayer->BeginFrame(frameData, callbacks);
            m_editorLayer->EndFrame();
        }
    }
    JobManager::Get()->WaitForJobs();
}

/// Render this frame of output
void Game::RenderFrame()
{
    PROFILE_SCOPE(RenderFrame);

    if (m_uiCanvas)
        m_uiCanvas->Prepare();

    // Upload gizmo geometry to GPU before acquiring the command buffer
    if (m_editorLayer)
        m_editorLayer->PrepareGizmos(&m_renderer);

    SDL_GPUCommandBuffer* commandBuffer = m_renderer.BeginCommandBuffer();
    if (!commandBuffer)
        return;

    // ── Pass 1: Scene editor view (always) ────────────────────────────────
    // Renders actors using the editor camera into m_sceneEditorTarget.
    // The shader chosen depends on m_sceneViewMode.
    {
        m_editorCamera->SetWorldToCamera(m_sceneCameraMatrix);

        const SDL_FColor clearColor = { 0.08f, 0.08f, 0.10f, 1.f };

        // Pick which scene-target shader to use for the current draw mode.
        // WireframeOnShaded uses Lit here then adds the wire overlay in sub-pass B.
        auto PickSceneShader = [&]() -> Shader* {
            switch (m_sceneViewMode)
            {
                case SceneViewMode::Unlit:        return m_assetManager->GetShader("SceneUnlit");
                case SceneViewMode::Wireframe:    return m_assetManager->GetShader("SceneWire");
                case SceneViewMode::LightContrib: return m_assetManager->GetShader("SceneLightContrib");
                case SceneViewMode::VertexColor:
                    // Uncomment the line below once SceneVertexColor is enabled in LoadShaders
                    // and your meshes carry a COLOR0 vertex channel:
                    // return m_assetManager->GetShader("SceneVertexColor");
                    return m_assetManager->GetShader("SceneUnlit"); // fallback
                case SceneViewMode::WireframeOnShaded: // lit sub-pass; wire overlay added below
                case SceneViewMode::Lit:
                default:                          return m_assetManager->GetShader("SceneLit");
            }
        };

        // Sub-pass A: main shading pass
        {
            SDL_GPURenderPass* pass = m_renderer.ClearScreen(commandBuffer, clearColor, m_sceneEditorTarget);
            m_editorCamera->SetActive(commandBuffer);
            m_lighting->SetActive(m_editorCamera, commandBuffer);
            Shader* sceneShader = PickSceneShader();
            int sceneMode = static_cast<int>(m_sceneViewMode);
            TerrainChunk::SetGamePass(false);
            for (Actor* a : m_actors)
            {
                Shader* s = a->PickSceneShader(sceneMode);
                a->Draw(commandBuffer, pass, s ? s : sceneShader);
            }
            // Streaming terrain — pick scene-mode-appropriate shader
            if (m_terrainManager)
            {
                bool isWire = (m_sceneViewMode == SceneViewMode::Wireframe ||
                               m_sceneViewMode == SceneViewMode::WireframeOnShaded);
                Shader* terrainShader = m_assetManager->GetShader(
                    isWire ? "SceneWire_Terrain" : "Terrain");
                m_terrainManager->Draw(commandBuffer, pass, terrainShader);
            }
            // Gizmos drawn last so they appear on top of all scene geometry
            if (m_editorLayer && !m_isPlaying)
            {
                m_editorCamera->SetActive(commandBuffer); // re-push VP so gizmo shader has it
                m_editorLayer->RenderGizmos(commandBuffer, pass);
            }
            m_renderer.EndRenderPass(pass);
        }

        // Sub-pass B: wireframe overlay (WireframeOnShaded only).
        // Additively blended dim lines drawn on top of the lit pass without clearing.
        if (m_sceneViewMode == SceneViewMode::WireframeOnShaded)
        {
            Shader* wireOverlay = m_assetManager->GetShader("SceneWireOverlay");
            if (wireOverlay)
            {
                SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_sceneEditorTarget);
                m_editorCamera->SetActive(commandBuffer);
                m_lighting->SetActive(m_editorCamera, commandBuffer);
                for (Actor* a : m_actors)
                {
                    // Overlay sub-pass always needs wire lines — use the Wireframe mode shader.
                    Shader* s = a->PickSceneShader(static_cast<int>(SceneViewMode::Wireframe));
                    a->Draw(commandBuffer, pass, s ? s : wireOverlay);
                }
                // Streaming terrain wireframe overlay
                if (m_terrainManager)
                    m_terrainManager->Draw(commandBuffer, pass,
                        m_assetManager->GetShader("SceneWire_Terrain"));
                m_renderer.EndRenderPass(pass);
            }
        }

    }

    // ── Passes 2-9: Full game pipeline (only when playing) ────────────────
    // Renders into m_renderTarget (intermediate), bloom-processes it,
    // then composites crosshair + UI into m_gameTarget for the Game View panel.
    if (m_isPlaying)
    {
        // Pass 2: main scene → m_renderTarget (game camera)
        {
            SDL_GPURenderPass* pass = m_renderer.ClearScreen(
                commandBuffer, SDL_FColor(0.0f, 0.2f, 0.4f, 1.f), m_renderTarget);

            m_camera->SetActive(commandBuffer);
            m_lighting->SetActive(m_camera, commandBuffer);
            TerrainChunk::SetGamePass(true);
            for (Actor* a : m_actors)
                a->Draw(commandBuffer, pass);
            if (m_terrainManager)
                m_terrainManager->Draw(commandBuffer, pass);
            TerrainChunk::SetGamePass(false);

            m_renderer.EndRenderPass(pass);
        }

        // Pass 3: bloom mask → m_halfTarget
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_halfTarget);
            m_assetManager->GetShader("Bloom")->SetActive(pass);
            m_renderTarget->SetActive(pass, 0);
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        // Pass 4: downsample → m_qTarget1
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_qTarget1);
            m_assetManager->GetShader("Copy")->SetActive(pass);
            m_halfTarget->SetActive(pass, 0);
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        float texelW = 1.f / (m_renderer.GetScreenWidth()  / 4.f);
        float texelH = 1.f / (m_renderer.GetScreenHeight() / 4.f);

        // Pass 5: horizontal blur → m_qTarget2
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_qTarget2);
            m_assetManager->GetShader("BlurH")->SetActive(pass);
            m_qTarget1->SetActive(pass, 0);
            BlurConstants bc{ Vector2(1.f,0.f), Vector2(texelW,texelH) };
            SDL_PushGPUFragmentUniformData(commandBuffer, 0, &bc, sizeof(bc));
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        // Pass 6: vertical blur → m_qTarget1
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_qTarget1);
            m_assetManager->GetShader("BlurV")->SetActive(pass);
            m_qTarget2->SetActive(pass, 0);
            BlurConstants bc{ Vector2(0.f,1.f), Vector2(texelW,texelH) };
            SDL_PushGPUFragmentUniformData(commandBuffer, 0, &bc, sizeof(bc));
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        // Pass 7: copy scene → m_gameTarget
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_gameTarget);
            m_assetManager->GetShader("GameCopy")->SetActive(pass);
            m_renderTarget->SetActive(pass, 0);
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        // Pass 8: additive bloom → m_gameTarget
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_gameTarget);
            m_assetManager->GetShader("GameCopyAdd")->SetActive(pass);
            m_qTarget1->SetActive(pass, 0);
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        // Pass 9: crosshair → m_gameTarget
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_gameTarget);
            m_assetManager->GetShader("GameCrosshair")->SetActive(pass);
            CrosshairConstants cc;
            cc.c_screenSize = Vector2(static_cast<float>(m_renderer.GetScreenWidth()),
                                      static_cast<float>(m_renderer.GetScreenHeight()));
            cc._pad[0] = cc._pad[1] = 0.f;
            SDL_PushGPUFragmentUniformData(commandBuffer, 0, &cc, sizeof(cc));
            m_vertexBuffer->Draw(commandBuffer, pass);
            m_renderer.EndRenderPass(pass);
        }

        // Pass 10: UI overlay → m_gameTarget
        if (m_uiCanvas)
        {
            SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer, m_gameTarget);
            m_uiCanvas->Render(commandBuffer, pass, m_assetManager->GetShader("GameUI"));
            m_renderer.EndRenderPass(pass);
        }
    }

    // ── Final pass: ImGui editor UI over the backbuffer ───────────────────
    if (m_editorLayer)
    {
        m_editorLayer->Prepare(commandBuffer);
        SDL_GPURenderPass* pass = m_renderer.BeginRenderPass(commandBuffer);
        m_editorLayer->Render(commandBuffer, pass);
        m_renderer.EndRenderPass(pass);
    }

    m_renderer.EndCommandBuffer(commandBuffer);
}

void Game::ProcessEditorEvent(SDL_Event* event)
{
    if (m_editorLayer)
    {
        m_editorLayer->ProcessEvent(event);

        if (event->type == SDL_EVENT_DROP_FILE && event->drop.data)
            m_editorLayer->NotifyOsFileDrop(event->drop.data);
    }
}

void Game::SetPlaying(bool playing)
{
    m_isPlaying = playing;
    // Release mouse capture when stopping so ImGui regains control
    if (!playing)
        SetMouseCapture(false);
}

/// Check the current status of a single key on the keyboard
/// @param key the key to check as a SDL_Scancode (SDL_SCANCODE_UP)
/// @return true if the key is currently held down or false if it is not
bool Game::IsKeyHeld(int key) const
{
    if (key >= m_numKeys)
        return false;
    return m_keyState[key];
}

/// Returns the mouse movement since the previous frame
/// @return movement in normalized device coordinates [-1.0, 1.0]
Vector2 Game::GetRelativeMouse() const
{
    Vector2 move = m_relativeMouse;
    return move * Vector2(1.0f / m_renderer.GetScreenWidth(), 1.0f / m_renderer.GetScreenHeight());
}

/// Returns the current state of the mouse buttons.
/// Use SDL_BUTTON_MASK() to ask for the button(s) you are interested in.
/// @return a SDL_MouseButtonFlags
uint32_t Game::GetMouseButtons() const
{
    return m_mouseButtons;
}

/// Returns true only on the first frame a mouse button is pressed (not while held)
/// @param buttonMask a SDL_BUTTON_MASK() value, e.g. SDL_BUTTON_MASK(SDL_BUTTON_LEFT)
bool Game::IsMouseButtonJustPressed(uint32_t buttonMask) const
{
    return (m_mouseButtons & buttonMask) && !(m_lastMouseButtons & buttonMask);
}

/// Returns true only on the first frame a key is pressed (not while held)
bool Game::IsKeyJustPressed(int key) const
{
    if (key >= m_numKeys)
        return false;
    return m_keyState[key] && !m_lastKeyState[key];
}

/// Returns the raw mouse movement in pixels since the previous frame
Vector2 Game::GetRawRelativeMouse() const
{
    return m_relativeMouse;
}

/// Lock or release the cursor for mouse-look
void Game::SetMouseCapture(bool capture)
{
    SDL_SetWindowRelativeMouseMode(m_renderer.GetWindow(), capture);
}

/// Resize all GPU render targets and update camera projection to match the new window size
void Game::OnResize(int w, int h)
{
    // Guard: ignore if called before Init or if size hasn't changed
    if (!m_renderTarget || !m_camera || !m_sceneEditorTarget)
        return;
    if (w == m_renderer.GetScreenWidth() && h == m_renderer.GetScreenHeight())
        return;

    // Wait for the GPU to finish all in-flight work before releasing any textures
    SDL_WaitForGPUIdle(m_renderer.GetDevice());

    m_sceneEditorTarget->Resize(w,     h);
    m_gameTarget->Resize(       w,     h);
    m_renderTarget->Resize(     w,     h);
    m_halfTarget->Resize(       w / 2, h / 2);
    m_qTarget1->Resize(         w / 4, h / 4);
    m_qTarget2->Resize(         w / 4, h / 4);

    m_renderer.OnResize(w, h);
    m_camera->SetScreenSize(w, h);
    m_editorCamera->SetScreenSize(w, h);

    if (m_uiCanvas)
        m_uiCanvas->OnResize(w, h);
}

/// Load a level creating instances of all the actors listed
/// @param fileName the name of the level to load ("Assets/Levels/Level06.itplevel")
/// @return true on success or false on failure
bool Game::LoadLevel(const char* fileName)
{
    PROFILE_SCOPE(LoadLevel);
    m_currentLevelPath = fileName;

	std::ifstream file(fileName);
	if (!file.is_open())
	{
		return false;
	}

	std::stringstream fileStream;
	fileStream << file.rdbuf();
	std::string contents = fileStream.str();
	rapidjson::StringStream jsonStr(contents.c_str());
	rapidjson::Document doc;
	doc.ParseStream(jsonStr);

	if (!doc.IsObject())
	{
		return false;
	}

	std::string str = doc["metadata"]["type"].GetString();
	int ver = doc["metadata"]["version"].GetInt();

	// Check the metadata
	if (!doc["metadata"].IsObject() ||
		str != "itplevel" ||
		ver != 3)
	{
		return false;
	}

    auto& camera = doc["camera"];
    if (camera.IsObject())
    {
        Vector3 position;
        Quaternion rotation;
        GetVectorFromJSON(camera, "position", position);
        GetQuaternionFromJSON(camera, "rotation", rotation);
        //build camera to world then invert
        Matrix4 camToWorld = Matrix4::CreateFromQuaternion(rotation)
        * Matrix4::CreateTranslation(position);
        Matrix4 worldToCam = camToWorld;
        worldToCam.Invert();
        m_camera->SetWorldToCamera(worldToCam);
    }

    auto& lighting = doc["lightingData"];
    if (lighting.IsObject())
    {
        Vector3 ambient;
        GetVectorFromJSON(lighting, "ambient", ambient);
        m_lighting->SetAmbientLight(&ambient);
    }

    auto& objs = doc["actors"];
    if (objs.IsArray())
    {
        for (rapidjson::SizeType i = 0; i < objs.Size(); i++)
        {
            if (!objs[i].IsObject()) continue;

            // Terrain actor — special type with GenParams instead of a mesh path
            std::string actorType;
            GetStringFromJSON(objs[i], "type", actorType);
            if (actorType == "terrain")
            {
                TerrainActor* ta = new TerrainActor();
                std::string terrainName;
                GetStringFromJSON(objs[i], "name", terrainName);
                if (!terrainName.empty()) ta->SetName(terrainName);

                TerrainChunk::GenParams p;
                if (objs[i].HasMember("resolution"))  p.resolution  = objs[i]["resolution"].GetInt();
                if (objs[i].HasMember("tileSize"))     p.tileSize    = objs[i]["tileSize"].GetFloat();
                if (objs[i].HasMember("heightScale"))  p.heightScale = objs[i]["heightScale"].GetFloat();
                if (objs[i].HasMember("noiseScale"))   p.noiseScale  = objs[i]["noiseScale"].GetFloat();
                if (objs[i].HasMember("octaves"))      p.octaves     = objs[i]["octaves"].GetInt();
                if (objs[i].HasMember("persistence"))  p.persistence = objs[i]["persistence"].GetFloat();
                if (objs[i].HasMember("lacunarity"))   p.lacunarity  = objs[i]["lacunarity"].GetFloat();
                if (objs[i].HasMember("seed"))         p.seed        = objs[i]["seed"].GetInt();
                ta->SetParams(p);

                int loadRadius = 2;
                if (objs[i].HasMember("loadRadius"))   loadRadius = objs[i]["loadRadius"].GetInt();
                ta->GetManager()->SetLoadRadius(loadRadius);
                ta->GetManager()->SetEnabled(true);

                ta->SetShaders(
                    m_assetManager->GetShader("Terrain"),
                    m_assetManager->GetShader("TerrainGame"),
                    m_assetManager->GetShader("SceneWire_Terrain")
                );

                m_terrainActor = ta;
                m_actors.push_back(ta);
                continue;
            }

            {
            Vector3 position;
            Quaternion rotation;
            float scale = 1.0f;
            GetVectorFromJSON(objs[i], "position", position);
            GetQuaternionFromJSON(objs[i], "rotation", rotation);
            GetFloatFromJSON(objs[i], "scale", scale);
                //create mat
                Matrix4 mat = Matrix4::CreateScale(scale)
                * Matrix4::CreateFromQuaternion(rotation)
                * Matrix4::CreateTranslation(position);
                //load mesh
                std::string meshFile;
                GetStringFromJSON(objs[i], "mesh", meshFile);
                Mesh* mesh = m_assetManager->LoadMesh(meshFile);

                Actor* actor = nullptr;
                if (mesh && mesh->IsSkinned())
                {
                    SkinnedObj* sk = new SkinnedObj(mesh);
                    sk->SetSceneShaders(
                        m_assetManager->GetShader("SceneLit_Skinned"),
                        m_assetManager->GetShader("SceneWire_Skinned"),
                        m_assetManager->GetShader("SceneWire_Skinned")
                    );
                    actor = sk;
                }
                else
                    actor = new Actor(mesh);
                actor->SetTransform(mat);
                actor->SetMeshPath(meshFile);

                std::string actorName;
                GetStringFromJSON(objs[i], "name", actorName);
                if (actorName.empty())
                    actorName = "Actor_" + std::to_string(m_actors.size());
                actor->SetName(actorName);

                m_actors.push_back(actor);
                //loop through and load components
                const auto& components = objs[i]["components"];
                if (components.IsArray())
                {
                    for (rapidjson::SizeType i = 0; i < components.Size(); i++)
                    {
                        std::string type;
                        GetStringFromJSON(components[i], "type", type);
                        if (type == "PointLight")
                        {
                            PointLight* pointLight = new PointLight(actor, m_lighting);
                            pointLight->LoadProperties(components[i]);
                            actor->AddComponent(pointLight);
                        }
                        else if (type == "Character")
                        {
                            Character* character = new Character(actor, m_assetManager);
                            character->LoadProperties(components[i]);
                            actor->AddComponent(character);
                        }
                        else if (type == "Player")
                        {
                            // Player needs a SkinnedObj, so cast the actor
                            SkinnedObj* skinnedObj = dynamic_cast<SkinnedObj*>(actor);
                            if (skinnedObj)
                            {
                                Player* player = new Player(skinnedObj, this, m_assetManager);
                                player->LoadProperties(components[i]);
                                actor->AddComponent(player);
                            }
                        }
                        else if (type == "FollowCam")
                        {
                            FollowCam* followCam = new FollowCam(actor, this);
                            followCam->LoadProperties(components[i]);
                            actor->AddComponent(followCam);
                        }
                        else if (type == "FPSCamera")
                        {
                            FPSCamera* fpsCamera = new FPSCamera(actor, this);
                            fpsCamera->LoadProperties(components[i]);
                            actor->AddComponent(fpsCamera);
                        }
                        else if (type == "CollisionBox")
                        {
                            CollisionBox* box = new CollisionBox(actor);
                            box->LoadProperties(components[i]);
                            actor->AddComponent(box);
                        }
                        else if (type == "SimpleRotate")
                        {
                            SimpleRotate* simpleRotate = new SimpleRotate(actor);
                            simpleRotate->LoadProperties(components[i]);
                            actor->AddComponent(simpleRotate);
                        }
                        else if (type == "TargetMover")
                        {
                            TargetMover* mover = new TargetMover(actor);
                            mover->LoadProperties(components[i]);
                            actor->AddComponent(mover);
                        }
                    }
                }

            }
        }
    }
	return true;
}

void Game::EditorSpawnPrimitive(int type, Vector3 worldPos)
{
    Mesh* mesh = (type == 0)
        ? PrimitiveMesh::CreateCube  (m_assetManager, 50.f)
        : PrimitiveMesh::CreateSphere(m_assetManager, 50.f);

    if (!mesh) return;

    static int s_primCount = 0;
    std::string name = (type == 0 ? "Cube_" : "Sphere_") + std::to_string(s_primCount++);

    Actor* actor = new Actor(mesh);
    actor->SetName(name);
    actor->SetPosition(worldPos);
    actor->SetScale(Vector3(1.f, 1.f, 1.f));
    m_actors.push_back(actor);
    m_selectedActor = static_cast<int>(m_actors.size()) - 1;
}

void Game::EditorSpawnMesh(const char* path, Vector3 worldPos)
{
    Mesh* mesh = m_assetManager->LoadMesh(path);
    if (!mesh) return;

    std::string name = std::filesystem::path(path).stem().string();

    Actor* actor = nullptr;
    if (mesh->IsSkinned())
    {
        SkinnedObj* sk = new SkinnedObj(mesh);
        sk->SetSceneShaders(
            m_assetManager->GetShader("SceneLit_Skinned"),
            m_assetManager->GetShader("SceneWire_Skinned"),
            m_assetManager->GetShader("SceneWire_Skinned") // wire overlay reuses wire shader
        );
        actor = sk;
    }
    else
        actor = new Actor(mesh);

    actor->SetName(name);
    actor->SetMeshPath(path);
    actor->SetPosition(worldPos);
    actor->SetScale(Vector3(1.f, 1.f, 1.f));
    m_actors.push_back(actor);
    m_selectedActor = static_cast<int>(m_actors.size()) - 1;
}

void Game::EditorRemoveActor(int index)
{
    if (index < 0 || index >= (int)m_actors.size()) return;
    if (m_actors[index] == m_terrainActor)
        m_terrainActor = nullptr;
    delete m_actors[index];
    m_actors.erase(m_actors.begin() + index);
    if (m_selectedActor >= (int)m_actors.size())
        m_selectedActor = (int)m_actors.size() - 1;
}

void Game::EditorSpawnTerrain()
{
    if (!m_terrainManager) return;

    // Only one TerrainActor is allowed in the level at a time.
    // If one already exists, just select it.
    if (m_terrainActor)
    {
        for (int i = 0; i < (int)m_actors.size(); ++i)
        {
            if (m_actors[i] == m_terrainActor)
            {
                m_selectedActor = i;
                break;
            }
        }
        return;
    }

    TerrainActor* ta = new TerrainActor();
    ta->SetParams(m_terrainManager->GetParams());
    ta->SetShaders(
        m_assetManager->GetShader("Terrain"),
        m_assetManager->GetShader("TerrainGame"),
        m_assetManager->GetShader("SceneWire_Terrain")
    );
    // Copy load radius from preview manager
    ta->GetManager()->SetLoadRadius(m_terrainManager->GetLoadRadius());
    ta->GetManager()->SetEnabled(true);

    m_terrainActor = ta;
    m_actors.push_back(ta);
    m_selectedActor = static_cast<int>(m_actors.size()) - 1;

    // Disable preview streaming so it doesn't render on top of the actor.
    m_terrainManager->SetEnabled(false);
    m_terrainManager->UnloadAll();
}

bool Game::SaveLevel(const char* fileName)
{
    if (!fileName || fileName[0] == '\0') return false;

    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // metadata
    {
        rapidjson::Value meta(rapidjson::kObjectType);
        meta.AddMember("type",    rapidjson::Value("itplevel", alloc), alloc);
        meta.AddMember("version", 3, alloc);
        doc.AddMember("metadata", meta, alloc);
    }

    // camera — preserve original camera block (identity rotation, zero position for now)
    {
        rapidjson::Value cam(rapidjson::kObjectType);
        rapidjson::Value pos(rapidjson::kArrayType);
        pos.PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(64.f, alloc);
        rapidjson::Value rot(rapidjson::kArrayType);
        rot.PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(0.f, alloc).PushBack(1.f, alloc);
        cam.AddMember("position", pos, alloc);
        cam.AddMember("rotation", rot, alloc);
        doc.AddMember("camera", cam, alloc);
    }

    // lightingData — preserve current ambient (hardcoded from original level for now)
    {
        rapidjson::Value lighting(rapidjson::kObjectType);
        rapidjson::Value amb(rapidjson::kArrayType);
        amb.PushBack(0.6f, alloc).PushBack(0.6f, alloc).PushBack(0.6f, alloc);
        lighting.AddMember("ambient", amb, alloc);
        doc.AddMember("lightingData", lighting, alloc);
    }

    // actors
    {
        rapidjson::Value actors(rapidjson::kArrayType);

        for (const Actor* actor : m_actors)
        {
            // TerrainActor serialises as a special terrain entry (no mesh path)
            if (const TerrainActor* ta = dynamic_cast<const TerrainActor*>(actor))
            {
                const TerrainChunk::GenParams& p = ta->GetParams();
                rapidjson::Value obj(rapidjson::kObjectType);
                obj.AddMember("type", rapidjson::Value("terrain", alloc), alloc);
                obj.AddMember("name", rapidjson::Value(ta->GetName().c_str(), alloc), alloc);
                obj.AddMember("loadRadius",  ta->GetManager()->GetLoadRadius(), alloc);
                obj.AddMember("resolution",  p.resolution,  alloc);
                obj.AddMember("tileSize",    p.tileSize,    alloc);
                obj.AddMember("heightScale", p.heightScale, alloc);
                obj.AddMember("noiseScale",  p.noiseScale,  alloc);
                obj.AddMember("octaves",     p.octaves,     alloc);
                obj.AddMember("persistence", p.persistence, alloc);
                obj.AddMember("lacunarity",  p.lacunarity,  alloc);
                obj.AddMember("seed",        p.seed,        alloc);
                actors.PushBack(obj, alloc);
                continue;
            }

            // Skip actors with no mesh path (camera actor, runtime-only, etc.)
            if (actor->GetMeshPath().empty()) continue;

            rapidjson::Value obj(rapidjson::kObjectType);

            // position
            Vector3 p = actor->GetPosition();
            rapidjson::Value posArr(rapidjson::kArrayType);
            posArr.PushBack(p.x, alloc).PushBack(p.y, alloc).PushBack(p.z, alloc);
            obj.AddMember("position", posArr, alloc);

            // rotation — convert Euler degrees back to quaternion
            // Convention matches RebuildMatrix: RotZ(yaw) * RotX(roll) * RotY(pitch)
            Vector3 deg = actor->GetEulerDeg();
            Quaternion qx(Vector3(1,0,0), Math::ToRadians(deg.x));
            Quaternion qy(Vector3(0,1,0), Math::ToRadians(deg.y));
            Quaternion qz(Vector3(0,0,1), Math::ToRadians(deg.z));
            Quaternion q = Quaternion::Concatenate(Quaternion::Concatenate(qx, qy), qz);
            rapidjson::Value rotArr(rapidjson::kArrayType);
            rotArr.PushBack(q.x, alloc).PushBack(q.y, alloc)
                  .PushBack(q.z, alloc).PushBack(q.w, alloc);
            obj.AddMember("rotation", rotArr, alloc);

            // scale — write as uniform float (x component)
            Vector3 s = actor->GetScale();
            obj.AddMember("scale", s.x, alloc);

            // mesh path
            obj.AddMember("mesh", rapidjson::Value(actor->GetMeshPath().c_str(), alloc), alloc);

            // name
            obj.AddMember("name", rapidjson::Value(actor->GetName().c_str(), alloc), alloc);

            // components — empty for editor-spawned actors; preserve nothing for now
            obj.AddMember("components", rapidjson::Value(rapidjson::kArrayType), alloc);

            actors.PushBack(obj, alloc);
        }

        doc.AddMember("actors", actors, alloc);
    }

    // Write to file with pretty formatting
    std::ofstream outFile(fileName);
    if (!outFile.is_open()) return false;

    rapidjson::OStreamWrapper osw(outFile);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
    writer.SetIndent(' ', 2);
    doc.Accept(writer);

    SDL_Log("SaveLevel: wrote %s", fileName);
    return true;
}

/// Load all the shaders we will need
/// We load all shaders up front (Loading shaders takes a while and should not be done during the frame)
/// Ideally, we would pre-compile the shaders and load the binaries, but we'll be loading them as raw HLSL
void Game::LoadShaders()
{
    PROFILE_SCOPE(LoadShaders);

    {
        // Phong shader
        Shader* phongShader = new Shader(Renderer::Get(), "Shaders/Phong.hlsl");
        SDL_GPUVertexAttribute phongAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalUV, normal) },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosNormalUV, uv) }
        };
        phongShader->SetColorTarget(m_renderTarget); //major bug found on piazza manually setting the color target did fix the issue :)
        phongShader->CreatePipeline(phongAttribs, ARRAY_SIZE(phongAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("Phong", phongShader);

        // Unlit shader
        Shader* unlitShader = new Shader(Renderer::Get(), "Shaders/Unlit.hlsl");
        unlitShader->SetColorTarget(m_renderTarget);
        unlitShader->CreatePipeline(phongAttribs, ARRAY_SIZE(phongAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("Unlit", unlitShader);

        //skinned shader
        Shader* skinnedShader = new Shader(Renderer::Get(), "Shaders/Skinned.hlsl");
        SDL_GPUVertexAttribute skinnedAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,      offsetof(SkinnedVertex, pos)          },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,      offsetof(SkinnedVertex, normal)       },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4,      offsetof(SkinnedVertex, boneIndices)  },
            { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(SkinnedVertex, boneWeights)  },
            { 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,      offsetof(SkinnedVertex, uv)           }
        };
        skinnedShader->SetColorTarget(m_renderTarget);
        skinnedShader->CreatePipeline(skinnedAttribs, ARRAY_SIZE(skinnedAttribs), sizeof(SkinnedVertex));
        m_assetManager->SetShader("Skinned", skinnedShader);

        // Scene-view variants of the skinned shader (target m_sceneEditorTarget).
        // Used by SkinnedObj::PickSceneShader so the correct vertex format is bound.
        Shader* sceneLitSkinned = new Shader(Renderer::Get(), "Shaders/SceneLit_Skinned.hlsl");
        sceneLitSkinned->SetColorTarget(m_sceneEditorTarget);
        sceneLitSkinned->CreatePipeline(skinnedAttribs, ARRAY_SIZE(skinnedAttribs), sizeof(SkinnedVertex));
        m_assetManager->SetShader("SceneLit_Skinned", sceneLitSkinned);

        Shader* sceneWireSkinned = new Shader(Renderer::Get(), "Shaders/SceneWire_Skinned.hlsl");
        sceneWireSkinned->SetColorTarget(m_sceneEditorTarget);
        sceneWireSkinned->SetFillMode(SDL_GPU_FILLMODE_LINE);
        sceneWireSkinned->SetCullMode(SDL_GPU_CULLMODE_NONE);
        sceneWireSkinned->CreatePipeline(skinnedAttribs, ARRAY_SIZE(skinnedAttribs), sizeof(SkinnedVertex));
        m_assetManager->SetShader("SceneWire_Skinned", sceneWireSkinned);

        //skinned shader
        Shader* normalShader = new Shader(Renderer::Get(), "Shaders/Normal.hlsl");
        SDL_GPUVertexAttribute normalAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,      offsetof(VertexPosNormalTangentUV, pos)          },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,      offsetof(VertexPosNormalTangentUV, normal)       },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,      offsetof(VertexPosNormalTangentUV, tangent)  },
            { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosNormalTangentUV, uv)  }
        };
        normalShader->SetColorTarget(m_renderTarget);
        normalShader->CreatePipeline(normalAttribs, ARRAY_SIZE(normalAttribs), sizeof(VertexPosNormalTangentUV));
        m_assetManager->SetShader("Normal", normalShader);

        //bloom shader
        Shader* bloomShader = new Shader(Renderer::Get(), "Shaders/BloomMask.hlsl");
        SDL_GPUVertexAttribute bloomAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };
        bloomShader->SetZWrite(false);
        bloomShader->SetZTest(false);
        bloomShader->SetColorTarget(m_halfTarget);
        bloomShader->CreatePipeline(bloomAttribs, ARRAY_SIZE(bloomAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("Bloom", bloomShader);

        //copy shader
        Shader* copyShader = new Shader(Renderer::Get(), "Shaders/Copy.hlsl");
        SDL_GPUVertexAttribute copyAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };
        copyShader->SetZWrite(false);
        copyShader->SetZTest(false);
        copyShader->SetColorTarget(m_qTarget1);
        copyShader->CreatePipeline(copyAttribs, ARRAY_SIZE(copyAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("Copy", copyShader);

        Shader* copyToScreenShader = new Shader(Renderer::Get(), "Shaders/Copy.hlsl");
        SDL_GPUVertexAttribute copyToScreenAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };
        copyToScreenShader->SetZWrite(false);
        copyToScreenShader->SetZTest(false);
        //no color target
        copyToScreenShader->CreatePipeline(copyToScreenAttribs, ARRAY_SIZE(copyToScreenAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("CopyToScreen", copyToScreenShader);

        //blue shaders
        SDL_GPUVertexAttribute blurAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };

        // horizontal blur — quarter 1 to quarter 2
        Shader* blurHShader = new Shader(Renderer::Get(), "Shaders/Blur.hlsl");
        blurHShader->SetZWrite(false);
        blurHShader->SetZTest(false);
        blurHShader->SetColorTarget(m_qTarget2);
        blurHShader->CreatePipeline(blurAttribs, ARRAY_SIZE(blurAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("BlurH", blurHShader);

        // vertical blur — quarter 2 to quarter 1
        Shader* blurVShader = new Shader(Renderer::Get(), "Shaders/Blur.hlsl");
        blurVShader->SetZWrite(false);
        blurVShader->SetZTest(false);
        blurVShader->SetColorTarget(m_qTarget1);
        blurVShader->CreatePipeline(blurAttribs, ARRAY_SIZE(blurAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("BlurV", blurVShader);

        // copyadd shader — additive blend, renders to back buffer
        Shader* copyAddShader = new Shader(Renderer::Get(), "Shaders/Copy.hlsl");
        SDL_GPUVertexAttribute copyAddAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };
        copyAddShader->SetZWrite(false);
        copyAddShader->SetZTest(false);
        copyAddShader->SetBlend(true, SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE); // additive
        // no SetColorTarget — back buffer
        copyAddShader->CreatePipeline(copyAddAttribs, ARRAY_SIZE(copyAddAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("CopyAdd", copyAddShader);
    }

    {
        // UI shader — screen-space 2D, alpha-blended, no depth test, renders to back buffer.
        // Vertex format: VertexUI { Vector2 pos, Vector2 uv, Color4 color }
        Shader* uiShader = new Shader(Renderer::Get(), "Shaders/UI.hlsl");
        SDL_GPUVertexAttribute uiAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexUI, pos)   },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexUI, uv)    },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(VertexUI, color) }
        };
        uiShader->SetZWrite(false);
        uiShader->SetZTest(false);
        uiShader->SetBlend(true,
            SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
        uiShader->CreatePipeline(uiAttribs, ARRAY_SIZE(uiAttribs), sizeof(VertexUI));
        m_assetManager->SetShader("UI", uiShader);
    }

    {
        // Crosshair overlay — alpha-blended, no depth test, renders to back buffer
        Shader* crosshairShader = new Shader(Renderer::Get(), "Shaders/Crosshair.hlsl");
        SDL_GPUVertexAttribute crosshairAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };
        crosshairShader->SetZWrite(false);
        crosshairShader->SetZTest(false);
        crosshairShader->SetBlend(true, SDL_GPU_BLENDFACTOR_SRC_ALPHA, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
        crosshairShader->CreatePipeline(crosshairAttribs, ARRAY_SIZE(crosshairAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("Crosshair", crosshairShader);
    }

    // Game-target variants: same shaders, pipeline points at m_gameTarget (RGBA8)
    // instead of the backbuffer so the full game pipeline renders into a texture.
    {
        SDL_GPUVertexAttribute uvAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosUV, pos) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosUV, uv)  }
        };

        Shader* gameCopy = new Shader(Renderer::Get(), "Shaders/Copy.hlsl");
        gameCopy->SetZWrite(false); gameCopy->SetZTest(false);
        gameCopy->SetColorTarget(m_gameTarget);
        gameCopy->CreatePipeline(uvAttribs, ARRAY_SIZE(uvAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("GameCopy", gameCopy);

        Shader* gameCopyAdd = new Shader(Renderer::Get(), "Shaders/Copy.hlsl");
        gameCopyAdd->SetZWrite(false); gameCopyAdd->SetZTest(false);
        gameCopyAdd->SetBlend(true, SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE);
        gameCopyAdd->SetColorTarget(m_gameTarget);
        gameCopyAdd->CreatePipeline(uvAttribs, ARRAY_SIZE(uvAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("GameCopyAdd", gameCopyAdd);

        Shader* gameCrosshair = new Shader(Renderer::Get(), "Shaders/Crosshair.hlsl");
        gameCrosshair->SetZWrite(false); gameCrosshair->SetZTest(false);
        gameCrosshair->SetBlend(true, SDL_GPU_BLENDFACTOR_SRC_ALPHA, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
        gameCrosshair->SetColorTarget(m_gameTarget);
        gameCrosshair->CreatePipeline(uvAttribs, ARRAY_SIZE(uvAttribs), sizeof(VertexPosUV));
        m_assetManager->SetShader("GameCrosshair", gameCrosshair);
    }
    {
        SDL_GPUVertexAttribute uiAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexUI, pos)   },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexUI, uv)    },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(VertexUI, color) }
        };
        Shader* gameUI = new Shader(Renderer::Get(), "Shaders/UI.hlsl");
        gameUI->SetZWrite(false); gameUI->SetZTest(false);
        gameUI->SetBlend(true, SDL_GPU_BLENDFACTOR_SRC_ALPHA, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
        gameUI->SetColorTarget(m_gameTarget);
        gameUI->CreatePipeline(uiAttribs, ARRAY_SIZE(uiAttribs), sizeof(VertexUI));
        m_assetManager->SetShader("GameUI", gameUI);
    }

    // ── Terrain shader ───────────────────────────────────────────────────────
    // Two pipeline instances: one targeting the scene editor RT (FLOAT32),
    // one targeting the game intermediate RT (FLOAT32). Same HLSL, different target.
    {
        SDL_GPUVertexAttribute terrainAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalColorUV, pos)    },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalColorUV, normal) },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(VertexPosNormalColorUV, color)  },
            { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosNormalColorUV, uv)     },
        };

        // Scene editor view variant
        Shader* terrainScene = new Shader(Renderer::Get(), "Shaders/Terrain.hlsl");
        terrainScene->SetColorTarget(m_sceneEditorTarget);
        terrainScene->SetCullMode(SDL_GPU_CULLMODE_NONE);
        terrainScene->CreatePipeline(terrainAttribs, ARRAY_SIZE(terrainAttribs), sizeof(VertexPosNormalColorUV));
        m_assetManager->SetShader("Terrain", terrainScene);

        // Game pipeline variant
        Shader* terrainGame = new Shader(Renderer::Get(), "Shaders/Terrain.hlsl");
        terrainGame->SetColorTarget(m_renderTarget);
        terrainGame->SetCullMode(SDL_GPU_CULLMODE_NONE);
        terrainGame->CreatePipeline(terrainAttribs, ARRAY_SIZE(terrainAttribs), sizeof(VertexPosNormalColorUV));
        m_assetManager->SetShader("TerrainGame", terrainGame);

        // Wireframe variant for scene-view debug mode (LINE fill, targets scene RT)
        Shader* terrainWire = new Shader(Renderer::Get(), "Shaders/SceneWire_Terrain.hlsl");
        terrainWire->SetColorTarget(m_sceneEditorTarget);
        terrainWire->SetFillMode(SDL_GPU_FILLMODE_LINE);
        terrainWire->SetCullMode(SDL_GPU_CULLMODE_NONE);
        terrainWire->CreatePipeline(terrainAttribs, ARRAY_SIZE(terrainAttribs), sizeof(VertexPosNormalColorUV));
        m_assetManager->SetShader("SceneWire_Terrain", terrainWire);

        // Give the terrain manager its shaders so streaming chunks render correctly.
        if (m_terrainManager)
            m_terrainManager->SetShaders(terrainScene, terrainGame, terrainWire);
    }

    // ── Scene-view debug shader variants ────────────────────────────────────
    // All target m_sceneEditorTarget (FLOAT32 RGBA). Selected in RenderFrame
    // Pass 1 based on m_sceneViewMode.
    {
        SDL_GPUVertexAttribute sceneAttribs[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalUV, pos)    },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalUV, normal) },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosNormalUV, uv)     },
        };

        // Lit — full Phong, targets scene editor RT
        Shader* sceneLit = new Shader(Renderer::Get(), "Shaders/Phong.hlsl");
        sceneLit->SetColorTarget(m_sceneEditorTarget);
        sceneLit->CreatePipeline(sceneAttribs, ARRAY_SIZE(sceneAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("SceneLit", sceneLit);

        // Unlit — diffuse texture only, no lighting
        Shader* sceneUnlit = new Shader(Renderer::Get(), "Shaders/Unlit.hlsl");
        sceneUnlit->SetColorTarget(m_sceneEditorTarget);
        sceneUnlit->CreatePipeline(sceneAttribs, ARRAY_SIZE(sceneAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("SceneUnlit", sceneUnlit);

        // Wireframe — flat grey PS, LINE fill, no culling so back-faces show as wire
        Shader* sceneWire = new Shader(Renderer::Get(), "Shaders/SceneWire.hlsl");
        sceneWire->SetColorTarget(m_sceneEditorTarget);
        sceneWire->SetFillMode(SDL_GPU_FILLMODE_LINE);
        sceneWire->SetCullMode(SDL_GPU_CULLMODE_NONE);
        sceneWire->CreatePipeline(sceneAttribs, ARRAY_SIZE(sceneAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("SceneWire", sceneWire);

        // Wireframe-on-Shaded overlay pass — dim grey lines additively blended on top of the Lit pass.
        // Drawn in a second sub-pass without clearing so the lit surface shows through.
        Shader* sceneWireOverlay = new Shader(Renderer::Get(), "Shaders/SceneWireOverlay.hlsl");
        sceneWireOverlay->SetColorTarget(m_sceneEditorTarget);
        sceneWireOverlay->SetFillMode(SDL_GPU_FILLMODE_LINE);
        sceneWireOverlay->SetCullMode(SDL_GPU_CULLMODE_NONE);
        sceneWireOverlay->SetZWrite(false);
        sceneWireOverlay->SetBlend(true, SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE);
        sceneWireOverlay->CreatePipeline(sceneAttribs, ARRAY_SIZE(sceneAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("SceneWireOverlay", sceneWireOverlay);

        // Light Contribution — diffuse lighting only, no texture
        Shader* sceneLightContrib = new Shader(Renderer::Get(), "Shaders/SceneLightContrib.hlsl");
        sceneLightContrib->SetColorTarget(m_sceneEditorTarget);
        sceneLightContrib->CreatePipeline(sceneAttribs, ARRAY_SIZE(sceneAttribs), sizeof(VertexPosNormalUV));
        m_assetManager->SetShader("SceneLightContrib", sceneLightContrib);

        // Vertex Color — outputs the raw COLOR0 vertex attribute.
        // IMPORTANT: This shader requires a vertex format that includes a COLOR0 channel.
        // VertexPosNormalUV (used by most engine meshes) does NOT have vertex colours.
        // Uncomment the block below once you have meshes / a vertex format with COLOR0.
        //
        // SDL_GPUVertexAttribute vcAttribs[] = {
        //     { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalColorUV, pos)    },
        //     { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(VertexPosNormalColorUV, normal) },
        //     { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(VertexPosNormalColorUV, color)  },
        //     { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(VertexPosNormalColorUV, uv)     },
        // };
        // Shader* sceneVertexColor = new Shader(Renderer::Get(), "Shaders/SceneVertexColor.hlsl");
        // sceneVertexColor->SetColorTarget(m_sceneEditorTarget);
        // sceneVertexColor->CreatePipeline(vcAttribs, ARRAY_SIZE(vcAttribs), sizeof(VertexPosNormalColorUV));
        // m_assetManager->SetShader("SceneVertexColor", sceneVertexColor);
    }
}
