//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SIMULATION_HPP
#define OPEN_GLASSBOX_SIMULATION_HPP

#  include "OpenGlassBox/ScriptParser.hpp"
#  include "OpenGlassBox/World.hpp"

//==============================================================================
//! \brief Entry point class managing (add, get, remove) a collection of Cities
//! and running simulation on them.
//! In this current phase of development Cities are not connected between them.
//!
//! A simulation is not a script: it runs cities, and it holds the script whose
//! types those cities refer to. Ask for script() to look a type up.
//==============================================================================
class Simulation
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
    //! \param[in] gridSizeU: how many cells a City spans along the U-axis by
    //! default.
    //! \param[in] gridSizeV: how many cells a City spans along the V-axis by
    //! default.
    //! \param[in] config: the runtime settings shared with every City.
    // -------------------------------------------------------------------------
    Simulation(uint32_t gridSizeU = 32u, uint32_t gridSizeV = 32u,
               SimulationConfig const& config = {});

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setListener(Simulation::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief The types the cities of this simulation refer to. It outlives the
    //! cities, which hold const references into it.
    // -------------------------------------------------------------------------
    Script const& script() const { return m_script; }
    Script& script() { return m_script; }

    // -------------------------------------------------------------------------
    //! \brief Update the game simulation.
    //! \param[in] deltaTime: the elapsed time in seconds since the previous
    //! update. It is scaled by timeScale() before being consumed.
    // -------------------------------------------------------------------------
    void update(float const deltaTime); // TODO: use std::chrono::ms

    // -------------------------------------------------------------------------
    //! \brief Run exactly one simulation tick, ignoring the accumulator, the
    //! pause state and the time scale. Used by the step-by-step debugger.
    // -------------------------------------------------------------------------
    void stepOneTick();

    // -------------------------------------------------------------------------
    //! \brief The ground the cities are founded on: the shared grid and the
    //! maps.
    // -------------------------------------------------------------------------
    World& world() { return m_world; }
    World const& world() const { return m_world; }

    // -------------------------------------------------------------------------
    //! \brief Getter/setter of the runtime settings, shared with every City.
    // -------------------------------------------------------------------------
    SimulationConfig const& config() const { return m_world.config(); }
    SimulationConfig& config() { return m_world.config(); }

    // -------------------------------------------------------------------------
    //! \brief Multiplier applied to the elapsed time given to update(). One
    //! means real time, two means twice as fast, zero freezes the simulation.
    // -------------------------------------------------------------------------
    float timeScale() const { return m_timeScale; }
    void setTimeScale(float scale) { m_timeScale = (scale < 0.0f) ? 0.0f : scale; }

    // -------------------------------------------------------------------------
    //! \brief When paused, update() accumulates no time and runs no tick.
    // -------------------------------------------------------------------------
    bool paused() const { return m_paused; }
    void setPaused(bool paused) { m_paused = paused; }

    // -------------------------------------------------------------------------
    //! \brief Number of ticks run since the beginning of the simulation.
    // -------------------------------------------------------------------------
    uint64_t totalTicks() const { return m_totalTicks; }

    SimulationClock const& clock() const { return m_world.clock(); }

    // -------------------------------------------------------------------------
    //! \brief Relative gap of the current assignment against the shortest
    //! paths at the current travel times: (TSTT - SPTT) / TSTT. Zero means
    //! every Agent is on a cheapest itinerary. Inspired by the Relgap of the
    //! MSA of CiudadSim.
    // -------------------------------------------------------------------------
    float relativeGap() const;

    // -------------------------------------------------------------------------
    //! \brief Found a new City in the world, replacing the one of the same name
    //! if it already exists. It spans the default number of cells given at
    //! construction.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f position);

    // -------------------------------------------------------------------------
    //! \brief Found a new City spanning the given number of cells.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f position, uint32_t sizeU,
                  uint32_t sizeV);

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
    Cities const& cities() const { return m_world.cities(); }
    Cities& cities() { return m_world.cities(); }

private:

    //! \brief Declared first so that it is destroyed after the world whose
    //! entities refer to its types.
    Script        m_script;
    //! \brief The maps, the grid and the cities.
    World         m_world;
    //! \brief How many cells a City spans when no size is given.
    uint32_t      m_gridSizeU;
    uint32_t      m_gridSizeV;
    //! \brief Accumulator of game time not yet consumed by a tick.
    float         m_time = 0.0f;
    //! \brief Multiplier applied to the elapsed time given to update().
    float         m_timeScale = 1.0f;
    //! \brief When true, update() is a no-op.
    bool          m_paused = false;
    //! \brief Number of ticks run since the beginning.
    uint64_t      m_totalTicks = 0u;
    Simulation::Listener *m_listener;
};

#endif
