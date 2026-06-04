#include "../pch.h"
#include "UIText.h"
#include "Font.h"
#include "UIRenderer.h"

void UIText::Draw(UIRenderer* renderer, float x, float y)
{
    if (!m_font || m_text.empty()) return;

    // xpos starts at the left edge; ypos is the baseline (top-left + ascent).
    float xpos = x;
    float ypos = y + m_font->GetAscent();

    Texture* atlas = m_font->GetAtlasTexture();

    for (char c : m_text)
    {
        float x0, y0, x1, y1, u0, v0, u1, v1;
        if (!m_font->GetGlyphQuad(c, &xpos, &ypos, &x0, &y0, &x1, &y1, &u0, &v0, &u1, &v1))
            continue; // skip unsupported characters

        float gw = x1 - x0;
        float gh = y1 - y0;
        if (gw > 0.f && gh > 0.f)
            renderer->SubmitQuad(x0, y0, gw, gh, u0, v0, u1, v1, m_color, atlas);
    }
}
