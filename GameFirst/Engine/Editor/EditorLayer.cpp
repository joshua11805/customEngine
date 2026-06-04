#include "../pch.h"
#include "EditorLayer.h"
#include "../Renderer.h"
#include "../Texture.h"
#include "../Actor.h"
#include "../EngineMath.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "imgui_internal.h"  // ImGui::DockBuilderXxx

bool EditorLayer::Init(Renderer* renderer)
{
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

void EditorLayer::BeginFrame(const EditorFrameData& data, const EditorCallbacks& callbacks)
{
    // Store frame data so panel helpers can access it without extra parameters
    m_actors         = data.actors;
    m_sceneEditorRT  = data.sceneEditorRT;
    m_gameRT         = data.gameRT;
    m_isPlaying      = data.isPlaying;
    m_selectedActor  = data.selectedActor;
    m_outSceneCamera = data.outSceneCamera;
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

        // Left 1/4: Hierarchy. Right 3/4: views + inspector.
        ImGuiID rightID, leftID;
        ImGui::DockBuilderSplitNode(dockID,  ImGuiDir_Left, 0.25f, &leftID, &rightID);

        // Right side: bottom 22% for Inspector, top 78% for Scene/Game tabs
        ImGuiID viewID, inspectorID;
        ImGui::DockBuilderSplitNode(rightID, ImGuiDir_Down, 0.22f, &inspectorID, &viewID);

        ImGui::DockBuilderDockWindow("Hierarchy",   leftID);
        // Scene View and Game View share the same tab group
        ImGui::DockBuilderDockWindow("Scene View",  viewID);
        ImGui::DockBuilderDockWindow("Game View",   viewID);
        ImGui::DockBuilderDockWindow("Inspector",   inspectorID);

        ImGui::DockBuilderFinish(dockID);
    }

    ImGui::End();

    if (m_showHierarchy)  DrawHierarchy();
    if (m_showSceneView)  DrawSceneView();
    if (m_showGameView)   DrawGameView();
    if (m_showInspector)  DrawInspector();
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

    if (ImGui::BeginMenu("File"))
    {
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
        ImGui::MenuItem("Scene View", nullptr, &m_showSceneView);
        ImGui::MenuItem("Game View",  nullptr, &m_showGameView);
        ImGui::MenuItem("Hierarchy",  nullptr, &m_showHierarchy);
        ImGui::MenuItem("Inspector",  nullptr, &m_showInspector);
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

void EditorLayer::DrawSceneView()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene View");
    ImGui::PopStyleVar();

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

        ImTextureID texID = static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(m_sceneEditorRT->GetTexture()));
        ImGui::Image(ImTextureRef(texID), ImVec2(dispW, dispH));

        // Overlay: navigation hint (bottom-left, fades into background)
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
        Matrix4 mat = actor->GetTransform();
        Vector3 pos = mat.GetTranslation();
        Vector3 scl = mat.GetScale();

        float posArr[3] = { pos.x, pos.y, pos.z };
        float sclArr[3] = { scl.x, scl.y, scl.z };

        ImGui::BeginDisabled();
        ImGui::DragFloat3("Position", posArr, 0.1f);
        ImGui::DragFloat3("Scale",    sclArr, 0.01f);
        ImGui::EndDisabled();
        ImGui::TextDisabled("(read-only)");
    }

    ImGui::End();
}
