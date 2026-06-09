#include "../pch.h"
#include "EditorLayer.h"
#include "../Renderer.h"
#include "../Texture.h"
#include "../Actor.h"
#include "../EngineMath.h"
#include "../TerrainManager.h"
#include "../TerrainActor.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "imgui_internal.h"  // ImGui::DockBuilderXxx

#include <filesystem>
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

bool EditorLayer::Init(Renderer* renderer, const char* projectRoot)
{
    if (projectRoot)
        m_projectRoot = projectRoot;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    // Softer dark theme so the scene view pops
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 4.0f;
    style.FrameRounding    = 3.0f;
    style.GrabRounding     = 3.0f;
    style.WindowBorderSize = 1.0f;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]         = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
    colors[ImGuiCol_Header]           = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]    = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderActive]     = ImVec4(0.24f, 0.24f, 0.27f, 1.00f);
    colors[ImGuiCol_Button]           = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered]    = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
    colors[ImGuiCol_ButtonActive]     = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg]          = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_Tab]              = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]       = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
    colors[ImGuiCol_TabSelected]      = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);

    ImGui_ImplSDL3_InitForSDLGPU(renderer->GetWindow());

    ImGui_ImplSDLGPU3_InitInfo info = {};
    info.Device             = renderer->GetDevice();
    info.ColorTargetFormat  = renderer->GetSwapchainFormat();
    info.MSAASamples        = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&info);

    return true;
}

void EditorLayer::Shutdown()
{
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::ProcessEvent(SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void EditorLayer::NotifyOsFileDrop(const char* path)
{
    m_osPendingDropPath = path ? path : "";
}

void EditorLayer::BeginFrame(const EditorFrameData& data, const EditorCallbacks& callbacks)
{
    // Store frame data so panel helpers can access it without extra parameters
    m_actors         = data.actors;
    m_sceneEditorRT  = data.sceneEditorRT;
    m_gameRT         = data.gameRT;
    m_isPlaying      = data.isPlaying;
    m_selectedActor  = data.selectedActor;
    m_outSceneCamera = data.outSceneCamera;
    m_sceneViewProj  = data.sceneViewProj;
    m_sceneViewMode  = data.sceneViewMode;
    m_terrainManager = data.terrainManager;
    m_terrainActor   = data.terrainActor;
    m_callbacks      = callbacks;

    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Full-window dockspace host
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking        |
        ImGuiWindowFlags_NoTitleBar       |
        ImGuiWindowFlags_NoCollapse       |
        ImGuiWindowFlags_NoResize         |
        ImGuiWindowFlags_NoMove           |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus       |
        ImGuiWindowFlags_NoBackground     |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##EditorHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    DrawMainMenuBar();

    ImGuiID dockID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockID, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    // Build the default layout once, the first time the dockspace appears.
    // ImGui persists dock positions in imgui.ini after the first run,
    // so we only force layout when the dockspace has no nodes yet.
    if (!m_layoutInitialized)
    {
        m_layoutInitialized = true;

        // Wipe any saved layout so we always start fresh from this split
        ImGui::DockBuilderRemoveNode(dockID);
        ImGui::DockBuilderAddNode(dockID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockID, viewport->WorkSize);

        // ┌──────────┬────────────────────────┬───────────┐
        // │Hierarchy │  Scene View / Game View │ Inspector │
        // │  left    ├────────────────────────┤   right   │
        // │  1/4     │     Project View        │   1/4     │
        // └──────────┴────────────────────────┴───────────┘

        // Carve left 1/4 for Hierarchy
        ImGuiID centreRightID, leftID;
        ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.20f, &leftID, &centreRightID);

        // Carve right 1/4 for Inspector
        ImGuiID centreID, rightID;
        ImGui::DockBuilderSplitNode(centreRightID, ImGuiDir_Right, 0.25f, &rightID, &centreID);

        // Split centre: top 70% views, bottom 30% project view
        ImGuiID viewID, projectID;
        ImGui::DockBuilderSplitNode(centreID, ImGuiDir_Down, 0.28f, &projectID, &viewID);

        ImGui::DockBuilderDockWindow("Hierarchy",    leftID);
        ImGui::DockBuilderDockWindow("Scene View",   viewID);
        ImGui::DockBuilderDockWindow("Game View",    viewID);
        ImGui::DockBuilderDockWindow("Project",      projectID);
        ImGui::DockBuilderDockWindow("Inspector",    rightID);

        ImGui::DockBuilderFinish(dockID);
    }

    ImGui::End();

    if (m_showHierarchy)    DrawHierarchy();
    if (m_showSceneView)    DrawSceneView();
    if (m_showGameView)     DrawGameView();
    if (m_showInspector)    DrawInspector();
    if (m_showProjectView)  DrawProjectView();
    if (m_showTerrainPanel) DrawTerrainPanel();
}

void EditorLayer::EndFrame()
{
    ImGui::Render();
}

void EditorLayer::Prepare(SDL_GPUCommandBuffer* cmd)
{
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData && drawData->DisplaySize.x > 0.0f && drawData->DisplaySize.y > 0.0f)
        ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmd);
}

void EditorLayer::Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass)
{
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData && drawData->DisplaySize.x > 0.0f && drawData->DisplaySize.y > 0.0f)
        ImGui_ImplSDLGPU3_RenderDrawData(drawData, cmd, pass);
}

bool EditorLayer::WantCaptureMouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool EditorLayer::WantCaptureKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

// -----------------------------------------------------------------------
// Panels
// -----------------------------------------------------------------------

void EditorLayer::DrawMainMenuBar()
{
    if (!ImGui::BeginMenuBar())
        return;

    // Ctrl+S hotkey — handled here so it works regardless of which panel has focus
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        if (m_callbacks.saveLevel)
            m_callbacks.saveLevel(m_callbacks.userData);
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Level", "Ctrl+S"))
        {
            if (m_callbacks.saveLevel)
                m_callbacks.saveLevel(m_callbacks.userData);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            SDL_Event quitEvent;
            SDL_zero(quitEvent);
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Scene View",   nullptr, &m_showSceneView);
        ImGui::MenuItem("Game View",    nullptr, &m_showGameView);
        ImGui::MenuItem("Hierarchy",    nullptr, &m_showHierarchy);
        ImGui::MenuItem("Inspector",    nullptr, &m_showInspector);
        ImGui::MenuItem("Project",      nullptr, &m_showProjectView);
        ImGui::MenuItem("Terrain",      nullptr, &m_showTerrainPanel);
        ImGui::EndMenu();
    }

    // Centred Play / Stop buttons
    const float buttonW = 80.0f;
    const float spacing = 4.0f;
    float totalW = buttonW * 2.f + spacing;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalW) * 0.5f);

    if (m_isPlaying)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.55f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.15f, 0.45f, 0.15f, 1.0f));
    }
    bool clickPlay = ImGui::Button("  Play  ", ImVec2(buttonW, 0));
    if (m_isPlaying) ImGui::PopStyleColor(3);
    if (clickPlay && !m_isPlaying && m_callbacks.setPlaying)
        m_callbacks.setPlaying(m_callbacks.userData, true);

    ImGui::SameLine(0.f, spacing);

    if (!m_isPlaying)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f, 0.14f, 0.14f, 1.0f));
    }
    bool clickStop = ImGui::Button("  Stop  ", ImVec2(buttonW, 0));
    if (!m_isPlaying) ImGui::PopStyleColor(3);
    if (clickStop && m_isPlaying && m_callbacks.setPlaying)
        m_callbacks.setPlaying(m_callbacks.userData, false);

    ImGui::EndMenuBar();
}

bool EditorLayer::ScreenToWorldGround(float pixelX, float pixelY,
                                      float panelX, float panelY, float panelW, float panelH,
                                      float groundZ, Vector3& outWorld) const
{
    if (panelW <= 0.f || panelH <= 0.f) return false;
    // Convert pixel to NDC [-1,1]
    float ndcX = ((pixelX - panelX) / panelW) * 2.f - 1.f;
    float ndcY = 1.f - ((pixelY - panelY) / panelH) * 2.f; // Y flipped

    // Unproject near and far points through the inverse VP matrix
    Matrix4 invVP = m_sceneViewProj;
    invVP.Invert();

    auto Unproject = [&](float z) -> Vector3 {
        // Homogeneous clip coords
        float clipX = ndcX, clipY = ndcY, clipZ = z, clipW = 1.f;
        // Multiply by inverse VP (row-major: p * M)
        const float* m = reinterpret_cast<const float*>(&invVP);
        float wx = clipX*m[0] + clipY*m[4] + clipZ*m[8]  + clipW*m[12];
        float wy = clipX*m[1] + clipY*m[5] + clipZ*m[9]  + clipW*m[13];
        float wz = clipX*m[2] + clipY*m[6] + clipZ*m[10] + clipW*m[14];
        float ww = clipX*m[3] + clipY*m[7] + clipZ*m[11] + clipW*m[15];
        if (fabsf(ww) < 1e-6f) ww = 1e-6f;
        return Vector3(wx/ww, wy/ww, wz/ww);
    };

    Vector3 near3 = Unproject(0.f);
    Vector3 far3  = Unproject(1.f);

    Vector3 dir = far3 - near3;
    float dz = dir.z;
    if (fabsf(dz) < 1e-6f) return false;

    float t = (groundZ - near3.z) / dz;
    if (t < 0.f) return false;

    outWorld = near3 + dir * t;
    outWorld.z = groundZ;
    return true;
}

void EditorLayer::DrawSceneView()
{
    // Toolbar above the scene image (needs window padding for buttons)
    ImGui::Begin("Scene View");

    // ── Toolbar: fixed-height row so the image gets the full remaining area ──
    {
        if (m_sceneViewMode)
        {
            static const char* k_modeNames[] = {
                "Lit",
                "Unlit",
                "Wireframe",
                "Wireframe on Shaded",
                "Light Contribution",
                "Vertex Color",
            };
            int current = static_cast<int>(*m_sceneViewMode);
            ImGui::SetNextItemWidth(180.f);
            if (ImGui::Combo("##DrawMode", &current, k_modeNames, IM_ARRAYSIZE(k_modeNames)))
                *m_sceneViewMode = static_cast<SceneViewMode>(current);
            ImGui::SameLine();
        }

        if (!m_isPlaying)
        {
            if (ImGui::Button("+ Cube"))
            {
                if (m_callbacks.spawnPrimitive)
                {
                    Vector3 spawnPos(0.f, 0.f, 50.f);
                    float cx = m_sceneViewX + m_sceneViewW * 0.5f;
                    float cy = m_sceneViewY + m_sceneViewH * 0.5f;
                    ScreenToWorldGround(cx, cy, m_sceneViewX, m_sceneViewY,
                                        m_sceneViewW, m_sceneViewH, 50.f, spawnPos);
                    m_callbacks.spawnPrimitive(m_callbacks.userData, 0, spawnPos);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Sphere"))
            {
                if (m_callbacks.spawnPrimitive)
                {
                    Vector3 spawnPos(0.f, 0.f, 50.f);
                    float cx = m_sceneViewX + m_sceneViewW * 0.5f;
                    float cy = m_sceneViewY + m_sceneViewH * 0.5f;
                    ScreenToWorldGround(cx, cy, m_sceneViewX, m_sceneViewY,
                                        m_sceneViewW, m_sceneViewH, 50.f, spawnPos);
                    m_callbacks.spawnPrimitive(m_callbacks.userData, 1, spawnPos);
                }
            }
            ImGui::SameLine();
            if (m_selectedActor && *m_selectedActor >= 0)
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.15f, 0.15f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.20f, 0.20f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.40f, 0.10f, 0.10f, 1.f));
                if (ImGui::Button("Delete Selected") && m_callbacks.removeActor)
                {
                    Actor* sel = (*m_actors)[*m_selectedActor];
                    if (sel == m_terrainActor) m_terrainActor = nullptr;
                    m_callbacks.removeActor(m_callbacks.userData, *m_selectedActor);
                    *m_selectedActor = -1;
                }
                ImGui::PopStyleColor(3);
            }
        }
    }
    // Separator so the image region starts on a fresh line with full width
    ImGui::Separator();

    bool panelHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

    // Update scene camera from mouse input while the panel is hovered
    if (!m_isPlaying)
        UpdateSceneCamera(panelHovered);

    if (m_sceneEditorRT && m_sceneEditorRT->GetTexture())
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float sceneW = static_cast<float>(m_sceneEditorRT->GetWidth());
        float sceneH = static_cast<float>(m_sceneEditorRT->GetHeight());
        float aspect = (sceneH > 0.f) ? sceneW / sceneH : 1.f;

        float dispW = avail.x;
        float dispH = avail.x / aspect;
        if (dispH > avail.y) { dispH = avail.y; dispW = avail.y * aspect; }

        float padX = (avail.x - dispW) * 0.5f;
        float padY = (avail.y - dispH) * 0.5f;
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + padX,
                                   ImGui::GetCursorPosY() + padY));

        // Record panel metrics for raycasting (used next frame by toolbar buttons)
        ImVec2 sp = ImGui::GetCursorScreenPos();
        m_sceneViewX = sp.x;
        m_sceneViewY = sp.y;
        m_sceneViewW = dispW;
        m_sceneViewH = dispH;

        ImTextureID texID = static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(m_sceneEditorRT->GetTexture()));
        ImGui::Image(ImTextureRef(texID), ImVec2(dispW, dispH));

        // ── Drag-and-drop target: accept mesh assets dropped from Project View ──
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH");
            if (payload && m_callbacks.spawnMesh)
            {
                // Raycast the drop position to ground plane
                ImVec2 dropPos = ImGui::GetIO().MousePos;
                Vector3 worldPos(0.f, 0.f, 0.f);
                ScreenToWorldGround(dropPos.x, dropPos.y,
                                    m_sceneViewX, m_sceneViewY, m_sceneViewW, m_sceneViewH,
                                    0.f, worldPos);
                std::string path(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                m_callbacks.spawnMesh(m_callbacks.userData, path.c_str(), worldPos);
            }
            ImGui::EndDragDropTarget();
        }

        // Overlay: navigation hint
        if (!m_isPlaying)
        {
            ImVec2 wPos = ImGui::GetWindowPos();
            ImVec2 wSz  = ImGui::GetWindowSize();
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            const char* hint = "RMB: look  |  MMB: pan  |  Scroll: zoom  |  Alt+LMB: orbit";
            ImVec2 ts = ImGui::CalcTextSize(hint);
            float hx = wPos.x + 8.f;
            float hy = wPos.y + wSz.y - ts.y - 8.f;
            dl->AddRectFilled(ImVec2(hx - 2, hy - 2), ImVec2(hx + ts.x + 2, hy + ts.y + 2),
                              IM_COL32(0, 0, 0, 140), 3.f);
            dl->AddText(ImVec2(hx, hy), IM_COL32(180, 180, 180, 200), hint);
        }

        // "PLAYING" badge
        if (m_isPlaying)
        {
            ImVec2 wPos = ImGui::GetWindowPos();
            ImVec2 wSz  = ImGui::GetWindowSize();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const char* label = "PLAYING";
            ImVec2 ts = ImGui::CalcTextSize(label);
            float bx = wPos.x + wSz.x - ts.x - 14.f;
            float by = wPos.y + 30.f;
            dl->AddRectFilled(ImVec2(bx - 4, by - 2), ImVec2(bx + ts.x + 4, by + ts.y + 2),
                              IM_COL32(30, 160, 30, 210), 3.f);
            dl->AddText(ImVec2(bx, by), IM_COL32(255, 255, 255, 255), label);
        }
    }
    else
    {
        ImGui::TextDisabled("(no scene render target)");
    }

    ImGui::End();
}

void EditorLayer::DrawGameView()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game View");
    ImGui::PopStyleVar();

    Texture* rt = m_isPlaying ? m_gameRT : nullptr;

    if (rt && rt->GetTexture())
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float aspect = (rt->GetHeight() > 0)
            ? static_cast<float>(rt->GetWidth()) / static_cast<float>(rt->GetHeight())
            : 1.f;

        float dispW = avail.x;
        float dispH = avail.x / aspect;
        if (dispH > avail.y) { dispH = avail.y; dispW = avail.y * aspect; }

        float padX = (avail.x - dispW) * 0.5f;
        float padY = (avail.y - dispH) * 0.5f;
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + padX,
                                   ImGui::GetCursorPosY() + padY));

        ImTextureID texID = static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(rt->GetTexture()));
        ImGui::Image(ImTextureRef(texID), ImVec2(dispW, dispH));
    }
    else
    {
        // Centre the message
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const char* msg = m_isPlaying ? "(no game render target)" : "Press Play to enter Game View";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        ImGui::SetCursorPos(ImVec2((avail.x - ts.x) * 0.5f, (avail.y - ts.y) * 0.5f));
        ImGui::TextDisabled("%s", msg);
    }

    ImGui::End();
}

// Build a world-to-camera matrix from the scene camera's yaw/pitch/position.
// Convention matches FPSCamera: yaw around Z, pitch around local Y.
Matrix4 EditorLayer::BuildSceneCameraMatrix() const
{
    // Forward vector from yaw and pitch
    Matrix4 rot = Matrix4::CreateRotationY(m_scenePitch) * Matrix4::CreateRotationZ(m_sceneYaw);
    // camera-to-world: rotation * translation
    Matrix4 camToWorld = rot * Matrix4::CreateTranslation(m_scenePos);
    camToWorld.Invert();
    return camToWorld;
}

void EditorLayer::UpdateSceneCamera(bool panelHovered)
{
    ImGuiIO& io = ImGui::GetIO();

    // We read raw ImGui mouse delta so we don't need SDL relative mode
    float dx = io.MouseDelta.x;
    float dy = io.MouseDelta.y;

    bool altHeld = io.KeyAlt;

    // Forward/right/up from current orientation
    Matrix4 rot   = Matrix4::CreateRotationY(m_scenePitch) * Matrix4::CreateRotationZ(m_sceneYaw);
    Vector3 fwd   = rot.GetXAxis();   // local X = world forward
    Vector3 right = rot.GetYAxis();   // local Y = world right
    Vector3 up    = rot.GetZAxis();   // local Z = world up

    // --- Right-mouse drag: rotate in place (flythrough look) ---
    if (panelHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.f))
    {
        const float sens = 0.004f;
        m_sceneYaw   += dx * sens;
        m_scenePitch += dy * sens;
        m_scenePitch  = Math::Clamp(m_scenePitch, -1.48f, 1.48f); // ~85 degrees
    }

    // --- Middle-mouse drag: pan (translate laterally) ---
    if (panelHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 1.f))
    {
        const float panSpeed = 0.5f;
        m_scenePos -= right * (dx * panSpeed);
        m_scenePos += up    * (dy * panSpeed);
    }

    // --- Alt + left-mouse drag: orbit around pivot ---
    if (panelHovered && altHeld && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.f))
    {
        const float sens = 0.004f;
        // Save pivot in world space
        Vector3 pivot = m_scenePos + fwd * m_scenePivotDist;

        m_sceneYaw   += dx * sens;
        m_scenePitch += dy * sens;
        m_scenePitch  = Math::Clamp(m_scenePitch, -1.48f, 1.48f);

        // Recompute forward after rotation and reposition at same pivot distance
        Matrix4 newRot = Matrix4::CreateRotationY(m_scenePitch) * Matrix4::CreateRotationZ(m_sceneYaw);
        Vector3 newFwd = newRot.GetXAxis();
        m_scenePos = pivot - newFwd * m_scenePivotDist;
    }

    // --- Scroll wheel: dolly (zoom along forward axis) ---
    if (panelHovered && io.MouseWheel != 0.f)
    {
        const float zoomSpeed = 30.f;
        m_scenePos += fwd * (io.MouseWheel * zoomSpeed);
        // Also shrink pivot distance so orbit stays sensible after zooming in
        m_scenePivotDist = Math::Max(10.f, m_scenePivotDist - io.MouseWheel * zoomSpeed);
    }

    // Write the resulting world-to-camera matrix out for Game to use
    if (m_outSceneCamera)
        *m_outSceneCamera = BuildSceneCameraMatrix();
}

void EditorLayer::DrawHierarchy()
{
    ImGui::Begin("Hierarchy");

    if (m_actors && m_selectedActor)
    {
        for (int i = 0; i < (int)m_actors->size(); ++i)
        {
            const std::string& name = (*m_actors)[i]->GetName();

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf           |
                ImGuiTreeNodeFlags_SpanFullWidth  |
                ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (i == *m_selectedActor)
                flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(i)),
                              flags, "%s", name.c_str());
            if (ImGui::IsItemClicked())
                *m_selectedActor = i;
        }

    }

    ImGui::End();
}

void EditorLayer::DrawInspector()
{
    ImGui::Begin("Inspector");

    if (!m_actors || !m_selectedActor || *m_selectedActor < 0 ||
        *m_selectedActor >= (int)m_actors->size())
    {
        ImGui::TextDisabled("Select an actor in the Hierarchy.");
        ImGui::End();
        return;
    }

    Actor* actor = (*m_actors)[*m_selectedActor];
    ImGui::TextUnformatted(actor->GetName().c_str());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Vector3 pos = actor->GetPosition();
        Vector3 rot = actor->GetEulerDeg();
        Vector3 scl = actor->GetScale();

        float posArr[3] = { pos.x, pos.y, pos.z };
        float rotArr[3] = { rot.x, rot.y, rot.z };
        float sclArr[3] = { scl.x, scl.y, scl.z };

        if (ImGui::DragFloat3("Position", posArr, 1.f))
            actor->SetPosition(Vector3(posArr[0], posArr[1], posArr[2]));

        if (ImGui::DragFloat3("Rotation", rotArr, 0.5f))
            actor->SetEulerDeg(Vector3(rotArr[0], rotArr[1], rotArr[2]));

        if (ImGui::DragFloat3("Scale", sclArr, 0.01f, 0.001f, 1000.f))
            actor->SetScale(Vector3(sclArr[0], sclArr[1], sclArr[2]));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Delete button
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.65f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.50f, 0.10f, 0.10f, 1.f));
    if (ImGui::Button("Delete Actor", ImVec2(-1, 0)) && m_callbacks.removeActor)
    {
        if (actor == m_terrainActor) m_terrainActor = nullptr;
        m_callbacks.removeActor(m_callbacks.userData, *m_selectedActor);
        *m_selectedActor = -1;
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
}

// -----------------------------------------------------------------------
// Project View
// -----------------------------------------------------------------------

// Returns a short label prefix that gives a visual hint about the file type.
static const char* FileIcon(const std::filesystem::path& p)
{
    if (std::filesystem::is_directory(p))         return "[D] ";
    std::string ext = p.extension().string();
    if (ext == ".cpp" || ext == ".cxx")           return "[C] ";
    if (ext == ".h"   || ext == ".hpp")           return "[H] ";
    if (ext == ".hlsl")                           return "[S] ";
    if (ext == ".png" || ext == ".jpg" ||
        ext == ".jpeg"|| ext == ".bmp")           return "[I] ";
    if (ext == ".gltf"|| ext == ".glb")           return "[M] ";
    if (ext == ".ttf" || ext == ".otf")           return "[F] ";
    if (ext == ".itplevel")                       return "[L] ";
    if (ext == ".json")                           return "[J] ";
    if (ext == ".itpmesh3")                       return "[M] ";
    return "[?] ";
}

static bool IsSourceFile(const std::filesystem::path& p)
{
    std::string ext = p.extension().string();
    return ext == ".cpp" || ext == ".cxx" || ext == ".h" || ext == ".hpp" || ext == ".hlsl";
}

static bool IsMeshFile(const std::filesystem::path& p)
{
    std::string ext = p.extension().string();
    return ext == ".gltf" || ext == ".glb" || ext == ".itpmesh3";
}

void EditorLayer::OpenInIDE(const std::filesystem::path& path)
{
#ifdef _WIN32
    // ShellExecuteW with "open" uses the OS file association —
    // CLion, VS Code, Notepad++, or whatever the user has registered for .cpp/.h
    std::wstring wpath = path.wstring();
    ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

void EditorLayer::CreateScript(const std::string& name, const std::filesystem::path& dir)
{
    namespace fs = std::filesystem;

    fs::path headerPath = dir / (name + ".h");
    fs::path sourcePath = dir / (name + ".cpp");

    // Write header
    {
        std::ofstream f(headerPath);
        f << "#pragma once\n"
          << "#include \"Component.h\"\n"
          << "\n"
          << "class " << name << " : public Component\n"
          << "{\n"
          << "public:\n"
          << "    " << name << "(Actor* owner);\n"
          << "    void Update(float deltaTime) override;\n"
          << "};\n";
    }

    // Write source
    {
        std::ofstream f(sourcePath);
        f << "#include \"pch.h\"\n"
          << "#include \"" << name << ".h\"\n"
          << "#include \"Actor.h\"\n"
          << "\n"
          << name << "::" << name << "(Actor* owner)\n"
          << "    : Component(owner)\n"
          << "{\n"
          << "}\n"
          << "\n"
          << "void " << name << "::Update(float deltaTime)\n"
          << "{\n"
          << "}\n";
    }

    // Open the header in the IDE immediately
    OpenInIDE(headerPath);
}

void EditorLayer::DrawDirectoryNode(const std::filesystem::path& path)
{
    namespace fs = std::filesystem;

    // Sort: directories first, then files, both alphabetically
    std::vector<fs::path> entries;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(path, ec))
        entries.push_back(e.path());
    std::sort(entries.begin(), entries.end(), [](const fs::path& a, const fs::path& b) {
        bool aDir = fs::is_directory(a);
        bool bDir = fs::is_directory(b);
        if (aDir != bDir) return aDir > bDir;
        return a.filename() < b.filename();
    });

    for (const fs::path& entry : entries)
    {
        std::string label = std::string(FileIcon(entry)) + entry.filename().string();
        bool isDir = fs::is_directory(entry);

        if (isDir)
        {
            bool open = ImGui::TreeNodeEx(label.c_str(),
                ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow);

            // Track which directory the mouse is hovering (for OS file drops)
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                m_projectHoveredDir = entry;

            // Accept OS file drops onto a directory node
            if (ImGui::BeginDragDropTarget())
            {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                    ImGui::GetDragDropPayload() ? ImGui::GetDragDropPayload()->DataType : "");
                (void)payload; // OS drops are handled via m_osPendingDropPath
                ImGui::EndDragDropTarget();
            }

            // Right-click a directory → context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("New Script here..."))
                {
                    m_newScriptTargetDir = entry;
                    m_newScriptName[0]   = '\0';
                    m_newScriptPopupOpen = true;
                }
                ImGui::EndPopup();
            }

            if (open)
            {
                DrawDirectoryNode(entry);
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TreeNodeEx(label.c_str(),
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_NoTreePushOnOpen);

            // Double-click a source file → open in IDE
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (IsSourceFile(entry))
                    OpenInIDE(entry);
            }

            // Drag mesh files into the Scene View
            if (IsMeshFile(entry) && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::string pathStr = entry.string();
                ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
                ImGui::Text("Drop into Scene View: %s", entry.filename().string().c_str());
                ImGui::EndDragDropSource();
            }

            // Right-click a source file → context menu
            if (IsSourceFile(entry) && ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Open in IDE"))
                    OpenInIDE(entry);
                ImGui::EndPopup();
            }
        }
    }
}

void EditorLayer::DrawProjectView()
{
    ImGui::Begin("Project");

    // ── Consume any pending OS file drop ────────────────────────────────────
    if (!m_osPendingDropPath.empty())
    {
        namespace fs = std::filesystem;
        fs::path src = m_osPendingDropPath;
        m_osPendingDropPath.clear();

        // Choose destination: last hovered dir, else Assets/
        fs::path destDir = (!m_projectHoveredDir.empty() && fs::is_directory(m_projectHoveredDir))
            ? m_projectHoveredDir
            : m_projectRoot / "Assets";

        fs::path dest = destDir / src.filename();

        std::error_code ec;
        if (!fs::exists(dest))
        {
            fs::copy_file(src, dest, ec);
            if (ec)
                SDL_Log("Project drop: copy failed: %s", ec.message().c_str());
            else
                SDL_Log("Project drop: copied %s → %s", src.string().c_str(), dest.string().c_str());
        }
        else
        {
            SDL_Log("Project drop: skipped — %s already exists", dest.string().c_str());
        }
    }

    // "New Script" button in the toolbar
    if (ImGui::Button("+ New Script"))
    {
        // Default to Game/Components/
        namespace fs = std::filesystem;
        fs::path componentsDir = m_projectRoot / "Game" / "Components";
        m_newScriptTargetDir = fs::exists(componentsDir) ? componentsDir
                                                         : m_projectRoot / "Game";
        m_newScriptName[0]   = '\0';
        m_newScriptPopupOpen = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Double-click a file to open  |  Right-click for options");
    ImGui::Separator();

    // Two root nodes: Assets/ and Game/
    namespace fs = std::filesystem;

    struct RootEntry { const char* label; fs::path path; };
    RootEntry roots[] = {
        { "[D] Assets", m_projectRoot / "Assets" },
        { "[D] Game",   m_projectRoot / "Game"   },
    };

    for (auto& root : roots)
    {
        if (!fs::exists(root.path)) continue;

        bool open = ImGui::TreeNodeEx(root.label,
            ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow);
        if (open)
        {
            DrawDirectoryNode(root.path);
            ImGui::TreePop();
        }
    }

    // ── New Script modal popup ────────────────────────────────────────────
    if (m_newScriptPopupOpen)
    {
        ImGui::OpenPopup("New Script");
        m_newScriptPopupOpen = false;
    }

    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("New Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create a new Component script in:");
        ImGui::TextDisabled("%s", m_newScriptTargetDir.string().c_str());
        ImGui::Separator();

        ImGui::SetNextItemWidth(-1.f);
        bool hitEnter = ImGui::InputText("##scriptname", m_newScriptName,
                                         sizeof(m_newScriptName),
                                         ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::TextDisabled("Class name (e.g. MyComponent)");

        ImGui::Spacing();

        bool nameValid = m_newScriptName[0] != '\0';

        if (!nameValid) ImGui::BeginDisabled();
        bool create = ImGui::Button("Create", ImVec2(120, 0)) || (hitEnter && nameValid);
        if (!nameValid) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        if (create)
        {
            CreateScript(std::string(m_newScriptName), m_newScriptTargetDir);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void EditorLayer::DrawTerrainPanel()
{
    ImGui::Begin("Terrain");

    if (!m_terrainManager)
    {
        ImGui::TextDisabled("No terrain manager available.");
        ImGui::End();
        return;
    }

    // ── If a TerrainActor is placed in the level, show its settings ─────────
    if (m_terrainActor)
    {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.f), "Active in level: Terrain");
        ImGui::Separator();

        TerrainChunk::GenParams params = m_terrainActor->GetParams();
        bool dirty = false;

        if (ImGui::CollapsingHeader("Mesh##ta", ImGuiTreeNodeFlags_DefaultOpen))
        {
            dirty |= ImGui::SliderInt("Resolution##ta",    &params.resolution,   8, 256);
            dirty |= ImGui::SliderFloat("Tile Size##ta",   &params.tileSize,     50.f, 2000.f);
            dirty |= ImGui::SliderFloat("Height Scale##ta",&params.heightScale,  10.f, 2000.f);
        }
        if (ImGui::CollapsingHeader("Noise##ta", ImGuiTreeNodeFlags_DefaultOpen))
        {
            dirty |= ImGui::SliderFloat("Frequency##ta",   &params.noiseScale,  0.0001f, 0.02f, "%.5f");
            dirty |= ImGui::SliderInt("Octaves##ta",       &params.octaves,     1, 10);
            dirty |= ImGui::SliderFloat("Persistence##ta", &params.persistence, 0.1f, 1.0f);
            dirty |= ImGui::SliderFloat("Lacunarity##ta",  &params.lacunarity,  1.0f, 4.0f);
            dirty |= ImGui::InputInt("Seed##ta",           &params.seed);
        }
        if (ImGui::CollapsingHeader("Streaming##ta", ImGuiTreeNodeFlags_DefaultOpen))
        {
            int radius = m_terrainActor->GetManager()->GetLoadRadius();
            if (ImGui::SliderInt("Load Radius (chunks)##ta", &radius, 0, 8))
                m_terrainActor->GetManager()->SetLoadRadius(radius);
            int total = (radius * 2 + 1) * (radius * 2 + 1);
            ImGui::TextDisabled("Max loaded: %d x %d = %d chunks", radius*2+1, radius*2+1, total);
        }

        if (dirty)
            m_terrainActor->GetManager()->Regenerate(params);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Select 'Terrain' in the Hierarchy to move it.");
        ImGui::End();
        return;
    }

    // ── No placed terrain yet — show preview controls ────────────────────────
    bool enabled = m_terrainManager->IsEnabled();
    if (ImGui::Checkbox("Enable Terrain", &enabled))
    {
        m_terrainManager->SetEnabled(enabled);
        if (!enabled)
            m_terrainManager->UnloadAll();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(preview in scene view)");

    ImGui::Separator();

    TerrainChunk::GenParams params = m_terrainManager->GetParams();
    bool dirty = false;

    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        dirty |= ImGui::SliderInt("Resolution",    &params.resolution,   8, 256);
        dirty |= ImGui::SliderFloat("Tile Size",   &params.tileSize,     50.f, 2000.f);
        dirty |= ImGui::SliderFloat("Height Scale",&params.heightScale,  10.f, 2000.f);
    }

    if (ImGui::CollapsingHeader("Noise", ImGuiTreeNodeFlags_DefaultOpen))
    {
        dirty |= ImGui::SliderFloat("Frequency",   &params.noiseScale,  0.0001f, 0.02f, "%.5f");
        dirty |= ImGui::SliderInt("Octaves",       &params.octaves,     1, 10);
        dirty |= ImGui::SliderFloat("Persistence", &params.persistence, 0.1f, 1.0f);
        dirty |= ImGui::SliderFloat("Lacunarity",  &params.lacunarity,  1.0f, 4.0f);
        dirty |= ImGui::InputInt("Seed",           &params.seed);
    }

    if (ImGui::CollapsingHeader("Streaming", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int radius = m_terrainManager->GetLoadRadius();
        if (ImGui::SliderInt("Load Radius (chunks)", &radius, 0, 6))
            m_terrainManager->SetLoadRadius(radius);
        int total = (radius * 2 + 1) * (radius * 2 + 1);
        ImGui::TextDisabled("Active chunks: %d x %d = %d", radius*2+1, radius*2+1, total);
    }

    if (dirty)
        m_terrainManager->Regenerate(params);

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Generate", ImVec2(-1, 0)))
    {
        if (m_callbacks.spawnTerrain)
            m_callbacks.spawnTerrain(m_callbacks.userData);
    }
    ImGui::TextDisabled("Adds a streaming Terrain actor to the level.");

    ImGui::End();
}
