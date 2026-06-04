#pragma once
#include "UIWidget.h"

class Texture;

// A solid-color or textured rectangle.
// Color is multiplied with the texture sample (use white for a pure texture).
class UIPanel : public UIWidget {
public:
    UIPanel() = default;

    // Set an optional texture. nullptr = solid color from m_color.
    void SetTexture(const Texture* tex) { m_texture = tex; }

protected:
    void Draw(UIRenderer* renderer, float x, float y) override;

private:
    const Texture* m_texture = nullptr;
};
