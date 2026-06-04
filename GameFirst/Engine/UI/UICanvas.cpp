#include "../pch.h"
#include "UICanvas.h"
#include "UIRenderer.h"
#include "UIWidget.h"
#include "../Renderer.h"
#include "../Shader.h"

bool UICanvas::Init(Renderer* renderer, int screenW, int screenH)
{
    m_renderer = renderer;
    m_screenW  = screenW;
    m_screenH  = screenH;

    m_uiRenderer = new UIRenderer();
    if (!m_uiRenderer->Init(renderer))
    {
        SDL_Log("UICanvas::Init — UIRenderer init failed");
        delete m_uiRenderer;
        m_uiRenderer = nullptr;
        return false;
    }
    return true;
}

void UICanvas::Shutdown()
{
    for (UIWidget* w : m_widgets)
        delete w;
    m_widgets.clear();

    if (m_uiRenderer)
    {
        m_uiRenderer->Shutdown();
        delete m_uiRenderer;
        m_uiRenderer = nullptr;
    }
}

void UICanvas::Update(float deltaTime)
{
    for (UIWidget* w : m_widgets)
        w->Update(deltaTime);
}

void UICanvas::Prepare()
{
    if (!m_uiRenderer) return;

    m_uiRenderer->BeginFrame();

    for (UIWidget* w : m_widgets)
        w->Collect(m_uiRenderer, m_screenW, m_screenH);

    m_uiRenderer->Upload(m_renderer);
}

void UICanvas::Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass, Shader* shader)
{
    if (!m_uiRenderer) return;
    m_uiRenderer->Render(cmd, pass, shader, m_screenW, m_screenH);
}

void UICanvas::AddWidget(UIWidget* widget)
{
    m_widgets.push_back(widget);
}
