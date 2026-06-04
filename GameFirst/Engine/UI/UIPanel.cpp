#include "../pch.h"
#include "UIPanel.h"
#include "UIRenderer.h"

void UIPanel::Draw(UIRenderer* renderer, float x, float y)
{
    renderer->SubmitQuad(x, y, m_w, m_h,
                         0.f, 0.f, 1.f, 1.f,
                         m_color, m_texture);
}
