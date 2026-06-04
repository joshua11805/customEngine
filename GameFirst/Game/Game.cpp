#include "pch.h"
#include "Game.h"
#include "../Engine/UI/UICanvas.h"
#include "../Engine/UI/UIPanel.h"
#include "../Engine/UI/UIText.h"
#include "../Engine/UI/Font.h"
#include "JsonUtil.h"
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
    m_renderTarget = new Texture();
    m_renderTarget->CreateRenderTarget(m_renderer.GetScreenWidth(), m_renderer.GetScreenHeight(), SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "FullSizeTarget");

    m_halfTarget = new Texture();
    m_halfTarget->CreateRenderTarget(m_renderer.GetScreenWidth()/2, m_renderer.GetScreenHeight()/2, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "HalfSizeTarget");

    m_qTarget1 = new Texture();
    m_qTarget1->CreateRenderTarget(m_renderer.GetScreenWidth()/4, m_renderer.GetScreenHeight()/4, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "FullSizeTarget");

    m_qTarget2 = new Texture();
    m_qTarget2->CreateRenderTarget(m_renderer.GetScreenWidth()/4, m_renderer.GetScreenHeight()/4, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, "FullSizeTarget");

    LoadShaders();

    {
        //create the camera
        m_camera = new Camera(&m_renderer);
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

    m_ticksCount = SDL_GetTicks();
    return true;
}

/// Shut down the game and release all resources
/// We do this in Shutdown BEFORE the destructor so that we can check for leaks
void Game::Shutdown()
{
    JobManager::Get()->End();

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

    m_assetManager->Clear();
    delete m_assetManager;
    m_assetManager = nullptr;

    delete m_physics;
    m_physics = nullptr;

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
        // Compute delta time
        uint64_t tickNow = SDL_GetTicks();

        // wait until 16 ms have elapsed at least
        while (tickNow - m_ticksCount < 16)
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

        //update all actors
        for (Actor* actor : m_actors)
        {
            actor->Update(deltaTime);
        }

        if (m_uiCanvas)
            m_uiCanvas->Update(deltaTime);

        if (m_fpsText && deltaTime > 0.f)
        {
            char buf[32];
            SDL_snprintf(buf, sizeof(buf), "FPS: %.0f", 1.0f / deltaTime);
            m_fpsText->SetText(buf);
        }
    }
    JobManager::Get()->WaitForJobs();
}

/// Render this frame of output
/// Remember, we are building a command buffer to send to the GPU - The GPU will do the actual rendering asynchronously
void Game::RenderFrame()
{
    PROFILE_SCOPE(RenderFrame);

    // Build and upload UI geometry before acquiring the frame command buffer.
    // UploadBuffer submits its own command buffer; doing this first guarantees
    // the vertex data is ready when the UI render pass executes.
    if (m_uiCanvas)
        m_uiCanvas->Prepare();

    // acquire the command buffer
    SDL_GPUCommandBuffer* commandBuffer = m_renderer.BeginCommandBuffer();
    if (nullptr == commandBuffer)
        return;

    {   // first render pass to offload onto offscreen texture
        SDL_GPURenderPass* renderPass = m_renderer.ClearScreen(commandBuffer, SDL_FColor(0.0f, 0.2f, 0.4f, 1.0f), m_renderTarget);

        {
            //set camera active
            m_camera->SetActive(commandBuffer);
            //set lighting active
            m_lighting->SetActive(m_camera, commandBuffer);

            //draw actors
            for (Actor* a : m_actors)
            {
                a->Draw(commandBuffer, renderPass);
            }
        }

        // end the render pass
        m_renderer.EndRenderPass(renderPass);
    }
    //second render pass - copy off-screen texture back to backbuffer
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer, m_halfTarget);

        //bind copy shader
        Shader* bloomMask = m_assetManager->GetShader("Bloom");
        bloomMask->SetActive(renderPass);

        //bind offscreen texture to slot 0
        m_renderTarget->SetActive(renderPass, 0);
        m_vertexBuffer->Draw(commandBuffer, renderPass);

        m_renderer.EndRenderPass(renderPass);
    }
    //pass #3 copy from halfsize to quarter size
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer, m_qTarget1);
        Shader* copy = m_assetManager->GetShader("Copy");
        copy->SetActive(renderPass);

        m_halfTarget->SetActive(renderPass, 0);
        m_vertexBuffer->Draw(commandBuffer, renderPass);

        m_renderer.EndRenderPass(renderPass);
    }

    //pass #4: horizontal blur
    float texelW = 1.0f / (m_renderer.GetScreenWidth() / 4.0f);
    float texelH = 1.0f / (m_renderer.GetScreenHeight() / 4.0f);
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer, m_qTarget2);
        Shader* blurH = m_assetManager->GetShader("BlurH");
        blurH->SetActive(renderPass);
        m_qTarget1->SetActive(renderPass, 0);

        // push horizontal blur constants
        BlurConstants blurConsts;
        blurConsts.c_blurDir   = Vector2(1.0f, 0.0f);  // horizontal
        blurConsts.c_texelSize = Vector2(texelW, texelH);
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &blurConsts, sizeof(blurConsts));

        m_vertexBuffer->Draw(commandBuffer, renderPass);
        m_renderer.EndRenderPass(renderPass);
    }
    //Pass #5: vertical blur
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer, m_qTarget1);
        Shader* blurV = m_assetManager->GetShader("BlurV");
        blurV->SetActive(renderPass);
        m_qTarget2->SetActive(renderPass, 0);

        // push vertical blur constants
        BlurConstants blurConsts;
        blurConsts.c_blurDir   = Vector2(0.0f, 1.0f);  // vertical
        blurConsts.c_texelSize = Vector2(texelW, texelH);
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &blurConsts, sizeof(blurConsts));

        m_vertexBuffer->Draw(commandBuffer, renderPass);
        m_renderer.EndRenderPass(renderPass);
    }

    //Pass 6: copy original scene to screen
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer);

        Shader* copyToScreen = m_assetManager->GetShader("CopyToScreen");
        copyToScreen->SetActive(renderPass);

        m_renderTarget->SetActive(renderPass, 0);
        m_vertexBuffer->Draw(commandBuffer, renderPass);

        m_renderer.EndRenderPass(renderPass);
    }

    //Pass 7: blend the blurred bloom back to the backbuffer
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer);
        Shader* copyAdd = m_assetManager->GetShader("CopyAdd");
        copyAdd->SetActive(renderPass);
        m_qTarget1->SetActive(renderPass, 0);
        m_vertexBuffer->Draw(commandBuffer, renderPass);
        m_renderer.EndRenderPass(renderPass);
    }
    //Pass 8: crosshair overlay
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer);
        Shader* crosshair = m_assetManager->GetShader("Crosshair");
        crosshair->SetActive(renderPass);
        CrosshairConstants cc;
        cc.c_screenSize = Vector2(static_cast<float>(m_renderer.GetScreenWidth()),
                                  static_cast<float>(m_renderer.GetScreenHeight()));
        cc._pad[0] = 0.0f;
        cc._pad[1] = 0.0f;
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &cc, sizeof(cc));
        m_vertexBuffer->Draw(commandBuffer, renderPass);
        m_renderer.EndRenderPass(renderPass);
    }
    //Pass 9: UI overlay
    if (m_uiCanvas)
    {
        SDL_GPURenderPass* renderPass = m_renderer.BeginRenderPass(commandBuffer);
        m_uiCanvas->Render(commandBuffer, renderPass, m_assetManager->GetShader("UI"));
        m_renderer.EndRenderPass(renderPass);
    }
    {   // submit the command buffer
        m_renderer.EndCommandBuffer(commandBuffer);
    }
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
    if (!m_renderTarget || !m_camera)
        return;
    if (w == m_renderer.GetScreenWidth() && h == m_renderer.GetScreenHeight())
        return;

    // Wait for the GPU to finish all in-flight work before releasing any textures
    SDL_WaitForGPUIdle(m_renderer.GetDevice());

    m_renderTarget->Resize(w,     h);
    m_halfTarget->Resize(  w / 2, h / 2);
    m_qTarget1->Resize(    w / 4, h / 4);
    m_qTarget2->Resize(    w / 4, h / 4);

    m_renderer.OnResize(w, h);
    m_camera->SetScreenSize(w, h);

    if (m_uiCanvas)
        m_uiCanvas->OnResize(w, h);
}

/// Load a level creating instances of all the actors listed
/// @param fileName the name of the level to load ("Assets/Levels/Level06.itplevel")
/// @return true on success or false on failure
bool Game::LoadLevel(const char* fileName)
{
    PROFILE_SCOPE(LoadLevel);

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
            if (objs[i].IsObject())
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
                    actor = new SkinnedObj(mesh);
                else
                    actor = new Actor(mesh);
                actor->SetTransform(mat);
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
}
