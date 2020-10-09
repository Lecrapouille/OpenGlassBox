//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef MAIN_HPP
#  define MAIN_HPP

#  include "Display/Window.hpp"
#  include "Core/Simulation.hpp"
#  include "Core/ScriptParser.hpp"

enum TextAlignement
{
    TEXT_LEFT,
    TEXT_CENTER,
    TEXT_RIGHT
};

class GlassBox: public IGame
{
public:

    GlassBox();

private:

    void blitRect(SDL_Texture *texture, SDL_Rect *src, int x, int y);
    void drawText(int x, int y, Uint8 r, Uint8 g, Uint8 b, TextAlignement align, const char *format, ...);
    bool initSDL(SDL_Renderer& renderer);
    bool initSimulation();
    virtual bool onInit(SDL_Renderer& renderer) override;
    virtual void onRelease(SDL_Renderer& renderer) override;
    virtual void onPaint(SDL_Renderer& renderer, float dt) override;
    virtual void onKeyDown(int key) override;

private:

    Simulation m_simulation;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture *m_fontTexture = nullptr;
    char m_drawTextBuffer[1024];
};

#endif
