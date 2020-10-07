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

class GlassBox: public IGame
{
public:

    GlassBox();
    virtual ~GlassBox() = default;

private:

    virtual bool onInit() override;
    virtual void onRelease() override;
    virtual void onPaint(SDL_Renderer& renderer, float dt) override;
    virtual void onKeyDown(int key) override;

private:

    Simulation m_simulation;
};

#endif
