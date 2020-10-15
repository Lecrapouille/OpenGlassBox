//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef MAIN_HPP
#  define MAIN_HPP

#  include "Display/IGame.hpp"
#  include "Core/Simulation.hpp"

//==============================================================================
//! \brief Entry point of the SimCity game.
//==============================================================================
class GlassBox: public IGame
{
public:

    GlassBox();

private:

    bool initSimulation();
    bool setupGraphics(SDL_Renderer& renderer);
    void renderSimulation(SDL_Renderer& renderer);
    void debugSimulation();
    void drawCity(City const& city, SDL_Renderer& renderer);
    void drawPaths(City const& city, SDL_Renderer& renderer);
    void drawMaps(City const& city, SDL_Renderer& renderer);
    void drawUnits(City const& city, SDL_Renderer& renderer);
    void drawAgents(City const& city, SDL_Renderer& renderer);
    virtual bool onInit(SDL_Renderer& renderer) override;
    virtual void onRelease(SDL_Renderer& renderer) override;
    virtual void onPaint(SDL_Renderer& renderer, float dt) override;
    virtual void onKeyDown(int key) override;

private:

    Simulation m_simulation;
    SDL_Texture *m_fontTexture = nullptr;
    bool m_debug_activated = false;
};

#endif
