//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef MAIN_HPP
#  define MAIN_HPP

#  include "Display/IGame.hpp"
#  include "Display/Window.hpp"
#  include "Display/DataPath.hpp"
#  include "OpenGlassBox/Simulation.hpp"

//==============================================================================
//! \brief Entry point of the SimCity game.
//==============================================================================
class GlassBox: public IGame,
                private City::Listener,
                private Simulation::Listener
{
public:

    GlassBox();
    bool run() { return m_window.run(*this); }

private:

    bool initSimulation(std::string const& simulation_file);
    bool setupGraphics(SDL_Renderer& renderer);
    void debugSimulation();

    void renderSimulation(SDL_Renderer& renderer);

    // --- TODO: to be static functions

    void drawCity(City const& city, SDL_Renderer& renderer);
    void drawPaths(City const& city, SDL_Renderer& renderer);
    void drawMaps(City const& city, SDL_Renderer& renderer);
    void drawUnits(City const& city, SDL_Renderer& renderer);
    void drawAgents(City const& city, SDL_Renderer& renderer);

    // --- Derived from IGame

    virtual bool onInit(SDL_Renderer& renderer) override;
    virtual void onRelease(SDL_Renderer& renderer) override;
    virtual void onPaint(SDL_Renderer& renderer, float dt) override;
    virtual void onKeyDown(int key) override;

    // --- Derived from City::Listener

    virtual void onMapAdded(Map& map) override;
    virtual void onMapRemoved(Map& map) override;
    virtual void onPathAdded(Path& path) override;
    virtual void onPathRemoved(Path& path) override;
    virtual void onUnitAdded(Unit& unit) override;
    virtual void onUnitRemoved(Unit& unit) override;
    virtual void onAgentAdded(Agent& agent) override;
    virtual void onAgentRemoved(Agent& agent) override;

    // --- Derived from Simulation::Listener

    virtual void onCityAdded(City& city) override;
    virtual void onCityRemoved(City& city) override;

private:

    Window       m_window;
    DataPath     m_path;
    Simulation   m_simulation;
    SDL_Texture *m_fontTexture = nullptr;
    bool         m_debug_activated = false;
    bool         m_pause = true;

    //! \brief Cache the name of Cities when added/removed
    //! to be used for ImGui::Combo.
    std::vector<std::string> m_cityNames;

    //! Selected city on the ImGui::Combo
    int m_city_combo_item = 0;
};

#endif