#pragma once
#include <vector>
#include "../EngineMath.h"

class UIRenderer;

// Abstract base for all UI elements.
// Position uses an anchor+offset+pivot system (all normalized 0-1 except offset which is pixels):
//   screenX = anchorX * screenW + offsetX - pivotX * width
//   screenY = anchorY * screenH + offsetY - pivotY * height
// Default (anchor=0,0, offset=0,0, pivot=0,0): pixel-exact top-left placement.
class UIWidget {
public:
    UIWidget()          = default;
    virtual ~UIWidget();

    // Override for per-frame logic (e.g., value animations).
    virtual void Update(float deltaTime);

    // Called by UICanvas to collect geometry. Resolves screen position, calls Draw(), recurses.
    void Collect(UIRenderer* renderer, int screenW, int screenH);

    // Position / layout
    void SetPosition(float x, float y)     { m_offsetX = x; m_offsetY = y; }
    void SetSize(float w, float h)          { m_w = w; m_h = h; }
    void SetAnchor(float ax, float ay)      { m_anchorX = ax; m_anchorY = ay; }
    void SetPivot(float px, float py)       { m_pivotX = px; m_pivotY = py; }
    void SetColor(const Color4& c)          { m_color = c; }
    void SetVisible(bool v)                 { m_visible = v; }
    bool IsVisible() const                  { return m_visible; }

    float GetWidth()  const { return m_w; }
    float GetHeight() const { return m_h; }

    void AddChild(UIWidget* child);

protected:
    // Subclasses submit quads here. x/y are the resolved top-left screen pixel position.
    virtual void Draw(UIRenderer* renderer, float x, float y) = 0;

    float  m_offsetX = 0.f, m_offsetY = 0.f;
    float  m_w = 100.f, m_h = 30.f;
    float  m_anchorX = 0.f, m_anchorY = 0.f;
    float  m_pivotX  = 0.f, m_pivotY  = 0.f;
    Color4 m_color   = {1.f, 1.f, 1.f, 1.f};
    bool   m_visible = true;

    std::vector<UIWidget*> m_children;
};
