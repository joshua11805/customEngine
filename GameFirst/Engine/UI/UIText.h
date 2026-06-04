#pragma once
#include "UIWidget.h"
#include <string>

class Font;

// Renders a string of text using a Font glyph atlas.
// Position is the top-left corner of the text bounds (baseline = y + font.ascent).
// Color controls the text tint; use (r,g,b,1) for fully opaque colored text.
class UIText : public UIWidget {
public:
    UIText() = default;

    void SetFont(Font* font)              { m_font = font; }
    void SetText(const std::string& text) { m_text = text; }

    const std::string& GetText() const    { return m_text; }

protected:
    void Draw(UIRenderer* renderer, float x, float y) override;

private:
    Font*       m_font = nullptr;
    std::string m_text;
};
