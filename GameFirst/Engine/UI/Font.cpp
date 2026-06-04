#include "../pch.h"
#include "Font.h"
#include "../Texture.h"
#include "../Renderer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

Font::Font() = default;

Font::~Font()
{
    delete[] static_cast<stbtt_bakedchar*>(m_charData);
    delete m_atlasTexture;
}

bool Font::Load(const char* fileName, float pixelHeight, Renderer* renderer)
{
    // Read the .ttf file into memory
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        SDL_Log("Font::Load — could not open %s", fileName);
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> ttfData(size);
    if (!file.read(reinterpret_cast<char*>(ttfData.data()), size))
    {
        SDL_Log("Font::Load — could not read %s", fileName);
        return false;
    }

    // Bake the glyph atlas
    std::vector<uint8_t> bitmap(ATLAS_W * ATLAS_H);
    auto* charData = new stbtt_bakedchar[NUM_CHARS];

    int result = stbtt_BakeFontBitmap(
        ttfData.data(), 0,
        pixelHeight,
        bitmap.data(), ATLAS_W, ATLAS_H,
        FIRST_CHAR, NUM_CHARS,
        charData);

    if (result <= 0)
    {
        SDL_Log("Font::Load — stbtt_BakeFontBitmap failed for %s (result=%d). "
                "Try a smaller pixelHeight or a larger atlas.", fileName, result);
        delete[] charData;
        return false;
    }

    // Extract font metrics for layout
    stbtt_fontinfo info;
    stbtt_InitFont(&info, ttfData.data(), stbtt_GetFontOffsetForIndex(ttfData.data(), 0));
    float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    m_ascent      = ascent  * scale;
    m_lineHeight  = (ascent - descent + lineGap) * scale;
    m_pixelHeight = pixelHeight;
    m_charData    = charData;

    // Upload atlas as RGBA (255,255,255,alpha) for the UI shader
    m_atlasTexture = new Texture();
    std::string atlasName = std::string(fileName) + "@" + std::to_string((int)pixelHeight);
    if (!m_atlasTexture->CreateFromAlpha(bitmap.data(), ATLAS_W, ATLAS_H, atlasName.c_str()))
    {
        SDL_Log("Font::Load — failed to create atlas texture for %s", fileName);
        delete m_atlasTexture;
        m_atlasTexture = nullptr;
        return false;
    }

    return true;
}

bool Font::GetGlyphQuad(char c, float* xpos, float* ypos,
                         float* x0, float* y0, float* x1, float* y1,
                         float* u0, float* v0, float* u1, float* v1) const
{
    if (!m_charData) return false;
    int idx = static_cast<int>(c) - FIRST_CHAR;
    if (idx < 0 || idx >= NUM_CHARS) return false;

    stbtt_aligned_quad q;
    // opengl_fillrule=0: D3D/Y-down convention — y0 < y1 (top < bottom on screen).
    stbtt_GetBakedQuad(
        static_cast<const stbtt_bakedchar*>(m_charData),
        ATLAS_W, ATLAS_H, idx,
        xpos, ypos, &q, 0);

    *x0 = q.x0; *y0 = q.y0;
    *x1 = q.x1; *y1 = q.y1;
    *u0 = q.s0; *v0 = q.t0;
    *u1 = q.s1; *v1 = q.t1;
    return true;
}
