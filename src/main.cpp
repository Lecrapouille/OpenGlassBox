//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "main.hpp"
#include "Display/SDLHelper.hpp"
#include "Display/DearImGui.hpp"
#include "Config.hpp"

//------------------------------------------------------------------------------
GlassBox::GlassBox()
    : m_simulation(config::SCREEN_WIDTH / config::GRID_SIZE,
                   config::SCREEN_HEIGHT / config::GRID_SIZE)
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

    m_window.setBackgroundColor(128, 128, 128);

    return true;
}

//------------------------------------------------------------------------------
bool GlassBox::initSimulation()
{
    m_simulation.setListener(*this);
    if (!m_simulation.parse("data/Simulations/TestCity.txt"))
        return false;

    City& paris = m_simulation.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    paris.setListener(*this);
    Path& road = paris.addPath(m_simulation.getPathType("Road"));
    Node& n1 = road.addNode(Vector3f(20.0f, 20.0f, 0.0f));
    Node& n2 = road.addNode(Vector3f(50.0f, 50.0f, 0.0f));
    Node& n3 = road.addNode(Vector3f(20.0f, 50.0f, 0.0f));
    Way& w1 = road.addWay(m_simulation.getWayType("Dirt"), n1, n2);
    Way& w2 = road.addWay(m_simulation.getWayType("Dirt"), n2, n3);
    Way& w3 = road.addWay(m_simulation.getWayType("Dirt"), n3, n1);
    Unit& u1 = paris.addUnit(m_simulation.getUnitType("Home"), road, w1, 0.66f);
    Unit& u2 = paris.addUnit(m_simulation.getUnitType("Home"), road, w1, 0.5f);
    Unit& u3 = paris.addUnit(m_simulation.getUnitType("Work"), road, w2, 0.5f);
    Unit& u4 = paris.addUnit(m_simulation.getUnitType("Work"), road, w3, 0.5f);
    Map& m1 = paris.addMap(m_simulation.getMapType("Grass"));
    Map& m2 = paris.addMap(m_simulation.getMapType("Water"));

    City& versailles = m_simulation.addCity("Versailles", Vector3f(1.0f, 0.0f, 0.0f));
    versailles.setListener(*this);
    Path& road2 = versailles.addPath(m_simulation.getPathType("Road"));
    Node& n4 = road2.addNode(Vector3f(40.0f, 20.0f, 0.0f));
    Way& w4 = road2.addWay(m_simulation.getWayType("Dirt"), n1, n4);

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
    default: break;
    }
}

//------------------------------------------------------------------------------
int main()
{
    GlassBox game;

    std::cout << "Press the key 'D' to show simulation debug" << std::endl;

    return game.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
