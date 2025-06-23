//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Display/SDLHelper.hpp"
#include <ctype.h>

#define GLYPH_WIDTH  (18)
#define GLYPH_HEIGHT (29)

//------------------------------------------------------------------------------
static void blitRect(SDL_Renderer* renderer, SDL_Texture* texture,
                     SDL_Rect* src, int x, int y)
{
    SDL_Rect dest;

    dest.x = x;
    dest.y = y;
    dest.w = src->w;
    dest.h = src->h;

    SDL_RenderCopy(renderer, texture, src, &dest);
}

//------------------------------------------------------------------------------
// Original code https://www.parallelrealities.co.uk/tutorials/
//------------------------------------------------------------------------------
void drawText(SDL_Renderer* renderer, SDL_Texture* fontTexture,
              int x, int y, Uint8 r, Uint8 g, Uint8 b,
              TextAlignment align, const char *format, ...)
{
    static char m_drawTextBuffer[1024];

    SDL_Rect rect;
    va_list args;

    memset(&m_drawTextBuffer, '\0', sizeof(m_drawTextBuffer));

    va_start(args, format);
    vsprintf(m_drawTextBuffer, format, args);
    va_end(args);

    int len = int(strlen(m_drawTextBuffer));

    switch (align)
    {
    case TextAlignment::TEXT_RIGHT:
        x -= (len * GLYPH_WIDTH);
        break;
    case TextAlignment::TEXT_CENTER:
        x -= (len * GLYPH_WIDTH) / 2;
        break;
    case TextAlignment::TEXT_LEFT:
    default:
        break;
    }

    rect.w = GLYPH_WIDTH;
    rect.h = GLYPH_HEIGHT;
    rect.y = 0;

    SDL_SetTextureColorMod(fontTexture, r, g, b);

    for (int i = 0 ; i < len ; i++)
    {
        int c = toupper(m_drawTextBuffer[i]);
        if (c >= ' ' && c <= 'Z')
        {
            rect.x = (c - ' ') * GLYPH_WIDTH;
            blitRect(renderer, fontTexture, &rect, x, y - GLYPH_HEIGHT);
            x += GLYPH_WIDTH;
        }
    }
}
