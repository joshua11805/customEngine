#include "../pch.h"
#include "UIWidget.h"
#include "UIRenderer.h"

UIWidget::~UIWidget()
{
    for (UIWidget* child : m_children)
        delete child;
}

void UIWidget::Update(float deltaTime)
{
    for (UIWidget* child : m_children)
        child->Update(deltaTime);
}

void UIWidget::Collect(UIRenderer* renderer, int screenW, int screenH)
{
    if (!m_visible) return;

    // Resolve anchor + offset + pivot to a top-left screen pixel coordinate.
    float x = m_anchorX * static_cast<float>(screenW) + m_offsetX - m_pivotX * m_w;
    float y = m_anchorY * static_cast<float>(screenH) + m_offsetY - m_pivotY * m_h;

    Draw(renderer, x, y);

    for (UIWidget* child : m_children)
        child->Collect(renderer, screenW, screenH);
}

void UIWidget::AddChild(UIWidget* child)
{
    m_children.push_back(child);
}
