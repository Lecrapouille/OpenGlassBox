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
bool GlassBox::initSimulation()
{
    m_simulation.setListener(*this);
    if (!m_simulation.parse("data/Simulations/TestCity.txt"))
        return false;

    City& paris = m_simulation.addCity("Paris", Vector3f(400.0f, 200.0f, 0.0f));
    paris.setListener(*this);
    Path& road = paris.addPath(m_simulation.getPathType("Road"));
    Node& n1 = road.addNode(Vector3f(60.0f, 60.0f, 0.0f) + paris.position());
    Node& n2 = road.addNode(Vector3f(300.0f, 300.0f, 0.0f) + paris.position());
    Node& n3 = road.addNode(Vector3f(60.0f, 300.0f, 0.0f) + paris.position());
    Way& w1 = road.addWay(m_simulation.getWayType("Dirt"), n1, n2);
    Way& w2 = road.addWay(m_simulation.getWayType("Dirt"), n2, n3);
    Way& w3 = road.addWay(m_simulation.getWayType("Dirt"), n3, n1);
    Unit& u1 = paris.addUnit(m_simulation.getUnitType("Home"), road, w1, 0.66f);
    Unit& u2 = paris.addUnit(m_simulation.getUnitType("Home"), road, w1, 0.5f);
    Unit& u3 = paris.addUnit(m_simulation.getUnitType("Work"), road, w2, 0.5f);
    Unit& u4 = paris.addUnit(m_simulation.getUnitType("Work"), road, w3, 0.5f);
    Map& m1 = paris.addMap(m_simulation.getMapType("Grass"));
    Map& m2 = paris.addMap(m_simulation.getMapType("Water"));

    City& versailles = m_simulation.addCity("Versailles", Vector3f(0.0f, 30.0f, 0.0f));
    versailles.setListener(*this);
    versailles.addMap(m_simulation.getMapType("Grass"));
    versailles.addMap(m_simulation.getMapType("Water"));
    Path& road2 = versailles.addPath(m_simulation.getPathType("Road"));
    Node& n4 = road2.addNode(Vector3f(40.0f, 20.0f, 0.0f) + versailles.position());
    Node& n5 = road2.addNode(Vector3f(200.0f, 200.0f, 0.0f) + versailles.position());
    Way& w4 = road2.addWay(m_simulation.getWayType("Dirt"), n4, n5);
    Way& w5 = road2.addWay(m_simulation.getWayType("Dirt"), n5, n1);
    Unit& u5 = versailles.addUnit(m_simulation.getUnitType("Work"), road2, w4, 0.9f);

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
        drawText(&renderer, m_fontTexture, 100u, 100u, 0u, 0u, 0u, TEXT_LEFT,
                 "Press P to play!");
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

//------------------------------------------------------------------------------
// Note: argc and argc are mandatory for Windows !
int main(int argc, char **argv)
{
    GlassBox game;

    std::cout << "Press the key 'D' to show simulation debug" << std::endl;

    return game.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
