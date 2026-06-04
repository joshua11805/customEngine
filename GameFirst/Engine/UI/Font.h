#pragma once
#include <string>
#include <cstdint>

class Texture;
class Renderer;

// Bakes a TTF font into a glyph atlas texture at a fixed pixel height.
// The atlas is stored as RGBA (255,255,255,alpha) for use with the UI shader.
class Font {
public:
    static constexpr int ATLAS_W     = 512;
    static constexpr int ATLAS_H     = 512;
    static constexpr int FIRST_CHAR  = 32;   // ASCII space
    static constexpr int NUM_CHARS   = 96;   // space through ~

    Font();
    ~Font();

    // Load a TTF file and bake it at the given pixel height.
    bool Load(const char* fileName, float pixelHeight, Renderer* renderer);

    Texture* GetAtlasTexture() const { return m_atlasTexture; }

    // Distance from the baseline to the top of the tallest glyph (positive, pixels).
    float GetAscent() const { return m_ascent; }

    // Total line height including leading (pixels).
    float GetLineHeight() const { return m_lineHeight; }

    // Get the screen-space quad for character c.
    // xpos/ypos: in/out cursor (xpos advances; ypos stays constant for single lines).
    // ypos should be set to the baseline position (widget.y + ascent).
    // Fills x0/y0 (top-left) and x1/y1 (bottom-right) of the quad in screen pixels.
    // Fills u0/v0 and u1/v1 with atlas UVs.
    // Returns false if c is outside the supported range.
    bool GetGlyphQuad(char c, float* xpos, float* ypos,
                      float* x0, float* y0, float* x1, float* y1,
                      float* u0, float* v0, float* u1, float* v1) const;

private:
    Texture* m_atlasTexture = nullptr;
    float    m_pixelHeight  = 0.0f;
    float    m_ascent       = 0.0f;
    float    m_lineHeight   = 0.0f;

    // Opaque storage for stbtt_bakedchar[NUM_CHARS] — avoids leaking the stb header.
    void* m_charData = nullptr;
};
