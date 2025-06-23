//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SIMULATION_HPP
#define OPEN_GLASSBOX_SIMULATION_HPP

#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/ScriptParser.hpp"

//==============================================================================
//! \brief Entry point class managing (add, get, remove) a collection of Cities
//! and running simulation on them.
//! In this current phase of development Cities are not connected between them.
//==============================================================================
class Simulation: public Script
{
public:

    class Listener
    {
    public:

        virtual ~Listener() = default;
        virtual void onCityAdded(City& /*city*/) {};
        virtual void onCityRemoved(City& /*city*/) {};
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Create a simulation game.
    //! \param[in] gridSizeU: the grid dimension along the U-axis for creating maps.
    //! \param[in] gridSizeV: the grid dimension along the V-axis for creating maps.
    // -------------------------------------------------------------------------
    Simulation(uint32_t gridSizeU = 32u, uint32_t gridSizeV = 32u);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setListener(Simulation::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief Update the game simulation.
    //! \param[in] deltaTime: the delta of time in ms from the previous update.
    // -------------------------------------------------------------------------
    void update(float const deltaTime); // TODO: use std::chrono::ms

    // -------------------------------------------------------------------------
    //! \brief Create a new City an replace the previous city if already exists.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f position);

    // -------------------------------------------------------------------------
    //! \brief Get the City referred by its name or throw an exception if the
    //! given name does not match any hold cities.
    // -------------------------------------------------------------------------
    City& getCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Get the City referred by its name or throw an exception if the
    //! given name does not match any hold cities.
    // -------------------------------------------------------------------------
    City const& getCity(std::string const& name) const;

    // -------------------------------------------------------------------------
    //! \brief Getter: return the collection of cities.
    // -------------------------------------------------------------------------
    Cities const& cities() const { return m_cities; }

private:

    uint32_t      m_gridSizeU;
    uint32_t      m_gridSizeV;
    float         m_time = 0.0f;
    Cities        m_cities;
    Simulation::Listener *m_listener;
};

#endif
