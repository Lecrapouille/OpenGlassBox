//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "main.hpp"
#include "Display/SDLHelper.hpp"
#include "Display/DearImGui.hpp"
#include "OpenGlassBox/Config.hpp"

//------------------------------------------------------------------------------
GlassBox::GlassBox()
    : m_simulation(12u, 12u)
{}

//------------------------------------------------------------------------------
void GlassBox::onRelease(SDL_Renderer&)
{
    SDL_DestroyTexture(m_fontTexture);
}

//------------------------------------------------------------------------------
bool GlassBox::setupGraphics(SDL_Renderer& renderer)
{
    const char *file = "data/Fonts/font.png";
    m_fontTexture = IMG_LoadTexture(&renderer, file);
    if (m_fontTexture == nullptr) {
        std::cerr << "Failed loading texture " << file << std::endl;
        return false;
    }

    m_window.setBackgroundColor(50, 50, 100);

    return true;
}

//------------------------------------------------------------------------------
bool GlassBox::onInit(SDL_Renderer& renderer)
{
    if (!initSimulation())
        return false;
    return setupGraphics(renderer);
}

//------------------------------------------------------------------------------
void GlassBox::onPaint(SDL_Renderer& renderer, float dt)
{
    if (m_pause)
    {
        drawText(&renderer, m_fontTexture, 50u, 100u, 0u, 0u, 0u, TEXT_LEFT,
                 "- Press P to play!");
        drawText(&renderer, m_fontTexture, 50u, 150u, 0u, 0u, 0u, TEXT_LEFT,
                 "- Press D to show debug during the play!");
        return ;
    }

    // Update states of the simulation
    // TODO: from a separated thread ?
    m_simulation.update(dt);

    renderSimulation(renderer);
    if (m_debug_activated) {
        debugSimulation();
    }
}

//------------------------------------------------------------------------------
void GlassBox::onKeyDown(int key)
{
    switch (key)
    {
    case SDL_SCANCODE_D:
        m_debug_activated ^= true;
        break;
    case SDL_SCANCODE_P:
        m_pause ^= true;
        break;
    default: break;
    }
}

#if !defined(_WIN32)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

//------------------------------------------------------------------------------
// Note: argc and argc are mandatory for SDL2 Windows !
int main(int argc, char **argv)
{
    GlassBox game;

    std::cout << "Press the key 'D' to show simulation debug" << std::endl;

    return game.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}

#if !defined(_WIN32)
#  pragma GCC diagnostic pop
#endif
