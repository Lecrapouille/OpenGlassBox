//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Simulation.hpp
//! \brief Simulation entry point: cities, time control and attached script.

#ifndef OPEN_GLASSBOX_SIMULATION_HPP
#define OPEN_GLASSBOX_SIMULATION_HPP

#include "OpenGlassBox/ScriptParser.hpp"
#include "OpenGlassBox/World.hpp"

#include <limits>

namespace ogb
{

//==============================================================================
//! \brief The whole game: the ruleset, the world it applies to, the towns
//! founded in it, and the clock driving all of them.
//!
//! This is where a program starts. A Simulation holds the ruleset parsed from a
//! script and the World the towns are founded in, in that order, so that the
//! ruleset outlives everything referring to it: a building keeps a reference to
//! the recipe it was built from, and that recipe lives in the ruleset.
//!
//! It also owns the wall clock to game time conversion. update() is handed the
//! seconds elapsed since the previous frame; it scales them by timeScale(),
//! accumulates them, and runs as many fixed ticks as fit. The simulation
//! therefore advances at the same rate whatever the frame rate, and a slow
//! machine drops behind rather than simulating differently.
//!
//! Towns are not connected to each other yet: an agent never leaves the town it
//! was sent out from. What they do share is the World, and therefore the layers
//! of the environment.
//!
//! Example:
//! \code
//! Simulation simulation(64u, 64u);
//! if (!simulation.script().parse("simulations/city.ogs"))
//!     return EXIT_FAILURE;
//!
//! City& paris = simulation.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
//!
//! // A game loop: real seconds in, fixed ticks out.
//! while (running)
//! {
//!     simulation.update(secondsSinceLastFrame);
//!     render(paris);
//! }
//! \endcode
//==============================================================================
class Simulation
{
public:

    //==========================================================================
    //! \brief What the renderer subscribes to in order to learn about towns
    //! being founded or dropped. The finer grained events of one town are on
    //! City::Listener.
    //==========================================================================
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //! \brief A town was founded.
        virtual void onCityAdded(City& /*city*/) {};

        //! \brief A town is about to go away, with everything it holds.
        virtual void onCityRemoved(City& /*city*/) {};
    };

public:

    // -------------------------------------------------------------------------
    //! \brief A game with an empty ruleset and no town yet.
    //! \param[in] gridSizeU how many cells a town spans along U when no size is
    //! given.
    //! \param[in] gridSizeV how many cells a town spans along V when no size is
    //! given.
    //! \param[in] config the runtime settings, copied and then shared with
    //! every town. Defaults to the values of the config namespace.
    // -------------------------------------------------------------------------
    explicit Simulation(uint32_t gridSizeU = 32u,
                        uint32_t gridSizeV = 32u,
                        SimulationConfig const& config = {});

    // -------------------------------------------------------------------------
    //! \brief Subscribe to towns being founded and dropped. Only one listener
    //! at a time: a second call replaces the first.
    //! \param[in] listener kept by address and has to outlive the Simulation.
    // -------------------------------------------------------------------------
    void setListener(Simulation::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief \return the ruleset the towns of this game refer to, and what a
    //! script is parsed into. It outlives the towns, which hold const
    //! references into it, so replacing it while a town is standing is not
    //! allowed.
    // -------------------------------------------------------------------------
    Script const& script() const
    {
        return m_script;
    }

    //! \copydoc script() const
    Script& script()
    {
        return m_script;
    }

    // -------------------------------------------------------------------------
    //! \brief Let the game catch up with the wall clock.
    //!
    //! The elapsed time is scaled by timeScale(), added to what was left over
    //! from the previous call, and spent one fixed tick at a time. What does
    //! not fill a tick is kept for next time, so no game time is lost or
    //! invented. Does nothing while paused.
    //!
    //! \param[in] deltaTime seconds elapsed since the previous call.
    // -------------------------------------------------------------------------
    void update(float const deltaTime); // TODO: use std::chrono::ms

    // -------------------------------------------------------------------------
    //! \brief Run exactly one tick, ignoring the leftover time, the pause and
    //! the time scale. What the step button of the debugger calls.
    // -------------------------------------------------------------------------
    void stepOneTick();

    // -------------------------------------------------------------------------
    //! \brief \return the ground the towns are founded on: the shared grid, the
    //! layers of the environment and the clock.
    // -------------------------------------------------------------------------
    World& world()
    {
        return m_world;
    }

    //! \copydoc world()
    World const& world() const
    {
        return m_world;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the runtime settings, shared with every town and safe to
    //! change while the game runs.
    // -------------------------------------------------------------------------
    SimulationConfig const& config() const
    {
        return m_world.config();
    }

    //! \copydoc config() const
    SimulationConfig& config()
    {
        return m_world.config();
    }

    // -------------------------------------------------------------------------
    //! \brief \return how much game time passes per second of wall time. One is
    //! real time, two is twice as fast, zero freezes the game without pausing
    //! it.
    // -------------------------------------------------------------------------
    float timeScale() const
    {
        return m_timeScale;
    }

    // -------------------------------------------------------------------------
    //! \brief \param[in] scale the new multiplier. Negative values are read as
    //! zero: time does not run backwards.
    // -------------------------------------------------------------------------
    void setTimeScale(float scale)
    {
        m_timeScale = (scale < 0.0f) ? 0.0f : scale;
    }

    // -------------------------------------------------------------------------
    //! \brief \return true while update() accumulates no time and runs no tick.
    //! The leftover time is kept, so unpausing resumes rather than restarts.
    // -------------------------------------------------------------------------
    bool paused() const
    {
        return m_paused;
    }

    //! \copydoc paused()
    void setPaused(bool paused)
    {
        m_paused = paused;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many ticks have been run since the beginning. What
    //! the clock of the game is derived from.
    // -------------------------------------------------------------------------
    uint64_t totalTicks() const
    {
        return m_totalTicks;
    }

    // -------------------------------------------------------------------------
    //! \brief Set the tick count outright, which only a save being loaded has
    //! any business doing: it is what puts the clock back where it was.
    //! \param[in] ticks the count to restore.
    // -------------------------------------------------------------------------
    void setTotalTicks(uint64_t ticks)
    {
        m_totalTicks = ticks;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the calendar of the game: the hour of the day and the day
    //! of the week the rules are given. Held by the World.
    // -------------------------------------------------------------------------
    SimulationClock const& clock() const
    {
        return m_world.clock();
    }

    //! \copydoc clock() const
    SimulationClock& clock()
    {
        return m_world.clock();
    }

    // -------------------------------------------------------------------------
    //! \brief How far the traffic is from equilibrium.
    //!
    //! The relative gap of the current assignment against the cheapest
    //! itineraries at the current travel times:
    //!
    //!     (TSTT - SPTT) / TSTT
    //!
    //! where TSTT is the total time actually spent travelling and SPTT the
    //! total time the same agents would spend on the cheapest itineraries. Zero
    //! means every agent is already on a cheapest itinerary, which is Wardrop
    //! equilibrium; a large value means the traffic is still settling. Borrowed
    //! from the Relgap of the method of successive averages of CiudadSim.
    //!
    //! Costs a whole graph search per Agent examined, so it is a diagnostic
    //! rather than something to call from the simulation itself. Two things
    //! keep it affordable when a panel reads it on every frame: the result is
    //! memoized until the next tick, and at most
    //! SimulationConfig::relativeGapSamples Agents are examined.
    //!
    //! \return the gap, or zero when nobody is on the road.
    // -------------------------------------------------------------------------
    float relativeGap() const;

    // -------------------------------------------------------------------------
    //! \brief Found a town spanning the default number of cells given at
    //! construction. A town of the same name is replaced.
    //! \param[in] name unique name of the town.
    //! \param[in] position world position of the top-left corner of its region.
    //! \return the new town.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f position);

    // -------------------------------------------------------------------------
    //! \brief Found a town spanning a given number of cells.
    //! \param[in] name unique name of the town.
    //! \param[in] position world position of the top-left corner of its region.
    //! \param[in] sizeU how many cells it administers along U.
    //! \param[in] sizeV how many cells it administers along V.
    //! \return the new town.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name,
                  Vector3f position,
                  uint32_t sizeU,
                  uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief \param[in] name name of the town.
    //! \return the town.
    //! \throw std::out_of_range when no town goes by that name.
    // -------------------------------------------------------------------------
    City& getCity(std::string const& name);

    //! \copydoc getCity(std::string const&)
    City const& getCity(std::string const& name) const;

    // -------------------------------------------------------------------------
    //! \brief \return the towns of the game, by name. Held by the World.
    // -------------------------------------------------------------------------
    Cities const& cities() const
    {
        return m_world.cities();
    }

    //! \copydoc cities() const
    Cities& cities()
    {
        return m_world.cities();
    }

private:

    //! \brief The ruleset. Declared before the world on purpose: destruction
    //! runs in reverse, so the world goes first and its buildings stop
    //! referring to these types before they are destroyed.
    Script m_script;
    //! \brief The layers, the grid, the clock and the towns.
    World m_world;
    //! \brief How many cells a town spans along U when no size is given.
    uint32_t m_gridSizeU;
    //! \brief How many cells a town spans along V when no size is given.
    uint32_t m_gridSizeV;
    //! \brief Game time accumulated but not yet spent on a tick. What keeps the
    //! rate of the simulation independent of the frame rate.
    float m_time = 0.0f;
    //! \brief How much game time passes per second of wall time.
    float m_timeScale = 1.0f;
    //! \brief While true, update() does nothing at all.
    bool m_paused = true;
    //! \brief How many ticks have been run since the beginning.
    uint64_t m_totalTicks = 0u;
    //! \brief Last value returned by relativeGap(), and the tick it was
    //! computed at. Mutable because the memoization has to happen inside a
    //! const method. The sentinel tick is one no run ever reaches, so the
    //! first call always computes.
    mutable float m_relativeGap = 0.0f;
    mutable uint64_t m_relativeGapTick = std::numeric_limits<uint64_t>::max();
    //! \brief Who to tell when a town is founded or dropped, or nullptr when
    //! nobody is listening. Not owned.
    Simulation::Listener* m_listener;
};

} // namespace ogb

#endif
