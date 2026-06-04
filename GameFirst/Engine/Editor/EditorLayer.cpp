#include "../pch.h"
#include "EditorLayer.h"
#include "../Renderer.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

bool EditorLayer::Init(Renderer* renderer)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

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

void EditorLayer::BeginFrame()
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Phase 1: show the full ImGui demo window to verify rendering
    ImGui::ShowDemoWindow();
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
