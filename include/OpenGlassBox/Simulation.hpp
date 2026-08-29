//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Simulation.hpp
//! \brief The way in: ruleset, cities, time control.

#ifndef OPEN_GLASSBOX_SIMULATION_HPP
#define OPEN_GLASSBOX_SIMULATION_HPP

#include "OpenGlassBox/Ruleset.hpp"
#include "OpenGlassBox/SimulationClock.hpp"
#include "OpenGlassBox/World.hpp"

#include <limits>

namespace ogb
{

//==============================================================================
//! \brief The whole game: the ruleset, the world it applies to, the cities
//! founded in it, and the clock driving all of them.
//!
//! This is where a program starts, and it is meant to be the only class an
//! application has to reach for. Everything the world holds is reachable from
//! here: layers, cities, the grid, the calendar. The World itself stays inside.
//!
//! A Simulation holds the ruleset and the World, in that order, so the ruleset
//! outlives everything referring to it: a building keeps a reference to the
//! recipe it was built from, and that recipe lives in the ruleset.
//!
//! It also turns real time into game time. update() is handed the seconds
//! elapsed since the previous frame; it scales them by getTimeScale(),
//! accumulates them, and runs as many fixed ticks as fit. The simulation
//! therefore advances at the same rate whatever the frame rate, and a slow
//! machine drops behind rather than simulating differently.
//!
//! Cities are not connected to each other yet: an agent never leaves the city
//! it was sent out from. What they do share is the grid, and therefore the
//! layers of the environment.
//!
//! Anything that changes the game goes through here, and anything that only
//! reads may go through the object that holds it. So a road is built with
//! addRoad() but read through getCity(), and a script is loaded with
//! loadScriptFile() but read through getRuleset().
//!
//! Example:
//! \code
//! ogb::Simulation simulation;
//! if (!simulation.loadScriptFile("simulations/city.ogs"))
//!     return EXIT_FAILURE;
//!
//! ogb::City& paris = simulation.addCity("Paris", { 0.0f, 0.0f, 0.0f });
//! ogb::installDijkstraRouter(paris, simulation.getConfig());
//! simulation.setPaused(false);
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

    //! \brief Runtime settings. See ogb::Config.
    using Config = ogb::Config;

    //! \brief Callbacks for cities appearing, going away, and roads crossing a
    //! border. What happens inside one city is on City::Listener.
    using Listener = SimulationListener;

    //==========================================================================
    //! \brief How far the traffic is from equilibrium, measured over a sample
    //! of the agents.
    //!
    //! The three numbers are worked out together and memoized until the next
    //! tick, so a panel may read them on every frame.
    //==========================================================================
    struct TrafficMetrics
    {
        //! \brief The relative gap of the current assignment against the
        //! cheapest itineraries at the current travel times:
        //!
        //!     (totalTravelTime - shortestPathTime) / totalTravelTime
        //!
        //! Zero means every agent is already on a cheapest itinerary, which is
        //! Wardrop equilibrium. A large value means the traffic is still
        //! settling. Zero too when nobody is on the road. Borrowed from the
        //! Relgap of the method of successive averages of CiudadSim.
        float relativeGap = 0.0f;
        //! \brief Time the sampled agents will actually spend travelling
        //! (TSTT).
        float totalTravelTime = 0.0f;
        //! \brief Time the same agents would spend on the cheapest itineraries
        //! (SPTT).
        float shortestPathTime = 0.0f;
    };

public:

    // -------------------------------------------------------------------------
    //! \brief A game with an empty ruleset and no city yet.
    //! \param[in] config the runtime settings, copied and then shared with
    //! every city. Defaults to the values of ogb::Config.
    // -------------------------------------------------------------------------
    explicit Simulation(Config const& config = {});

    Simulation(Simulation const&) = delete;
    Simulation& operator=(Simulation const&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Register the callbacks. One listener at a time: a second call
    //! replaces the first.
    //!
    //! Register before founding the first city, or the callback for it will be
    //! missed.
    //!
    //! \param[in] listener kept by address, has to outlive the Simulation.
    // -------------------------------------------------------------------------
    void setListener(Listener& listener);

    //--------------------------------------------------------------------------
    //! \name Ruleset
    //! @{
    //--------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief Load a script from a file, replacing what was loaded before.
    //!
    //! On failure the previous ruleset is kept, so a bad reload leaves a
    //! running game alone rather than emptying it.
    //!
    //! \param[in] filename the script to read. The language is picked from the
    //! extension.
    //! \return true on success. See getScriptErrors() for what went wrong.
    //!
    //! \note The cities already founded keep references into the previous
    //! ruleset. Load the script before founding anything.
    // -------------------------------------------------------------------------
    bool loadScriptFile(std::string const& filename);

    // -------------------------------------------------------------------------
    //! \brief Load a script held in memory. Used by the tests and by an editor
    //! that has the text but no file.
    //! \param[in] source the script itself.
    //! \param[in] name what the errors should call it, there being no path.
    //! \return true on success.
    // -------------------------------------------------------------------------
    bool loadScriptString(std::string const& source,
                          std::string const& name = "<string>");

    // -------------------------------------------------------------------------
    //! \brief \return everything found wrong by the last load, in the order it
    //! was found. Empty after a load that went well.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<ParseError> const& getScriptErrors() const
    {
        return m_ruleset.getErrors();
    }

    // -------------------------------------------------------------------------
    //! \brief \return the errors of the last load, one per line, ready to be
    //! shown.
    // -------------------------------------------------------------------------
    std::string formatScriptErrors() const
    {
        return m_ruleset.formatErrors();
    }

    // -------------------------------------------------------------------------
    //! \brief \return what the script declared: the recipes a city is built
    //! from. Read only, since the cities hold references into it.
    // -------------------------------------------------------------------------
    [[nodiscard]] Ruleset const& getRuleset() const
    {
        return m_ruleset;
    }

    //! @}

    //--------------------------------------------------------------------------
    //! \name Time
    //! @{
    //--------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief Let the game catch up with real time.
    //!
    //! The elapsed time is scaled by getTimeScale(), added to what was left
    //! over from the previous call, and spent one fixed tick at a time. What
    //! does not fill a tick is kept for next time, so no game time is lost or
    //! invented. Does nothing while paused.
    //!
    //! \param[in] deltaTime seconds elapsed since the previous call.
    // -------------------------------------------------------------------------
    void update(float deltaTime); // TODO: use std::chrono::ms

    // -------------------------------------------------------------------------
    //! \brief Run exactly one tick, ignoring the leftover time, the pause and
    //! the time scale. This is what the step button of the debugger calls.
    // -------------------------------------------------------------------------
    void stepOneTick();

    // -------------------------------------------------------------------------
    //! \brief \return how much game time passes per second of wall time. One is
    //! real time, two is twice as fast, zero freezes the game without pausing
    //! it.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getTimeScale() const
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
    //!
    //! \note A new Simulation starts paused, so an application can build its
    //! cities before anything moves.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isPaused() const
    {
        return m_paused;
    }

    // -------------------------------------------------------------------------
    //! \brief Pause or resume the simulation loop.
    //! \param[in] paused true to pause, false to resume.
    // -------------------------------------------------------------------------
    void setPaused(bool paused)
    {
        m_paused = paused;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the game calendar: day, hour and minute, and the tick
    //! count they come from. Read only: use setTicks() or setTimeOfDay() to
    //! move it.
    // -------------------------------------------------------------------------
    [[nodiscard]] SimulationClock const& getClock() const
    {
        return m_clock;
    }

    // -------------------------------------------------------------------------
    //! \brief Restore the tick counter. For loading a save.
    //! \param[in] ticks the counter to restore.
    // -------------------------------------------------------------------------
    void setTicks(uint64_t ticks)
    {
        m_clock.setTicks(ticks);
    }

    // -------------------------------------------------------------------------
    //! \brief Set the date. Called at construction with TimeConfig::startHour.
    //! \param[in] day whole days gone by, counted from zero.
    //! \param[in] hour hour of the day, in [0..23].
    //! \param[in] minute minute of the hour, in [0..59].
    // -------------------------------------------------------------------------
    void setTimeOfDay(uint32_t day, uint32_t hour, uint32_t minute)
    {
        m_clock.setTimeOfDay(day, hour, minute);
    }

    //! @}

    //--------------------------------------------------------------------------
    //! \name Settings
    //! @{
    //--------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief \return the runtime settings, shared with every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] Config const& getConfig() const
    {
        return m_world.getConfig();
    }

    // -------------------------------------------------------------------------
    //! \brief Replace the runtime settings while the game runs. Read the
    //! current ones, change what you need, hand them back.
    //!
    //! \param[in] config the new settings, copied.
    //! \note GridConfig::cellSize is read when a city is created and when it
    //! moves. Changing it under a city that already exists leaves its cells
    //! where they were.
    // -------------------------------------------------------------------------
    void setConfig(Config const& config)
    {
        m_world.setConfig(config);
    }

    //! @}

    //--------------------------------------------------------------------------
    //! \name Cities
    //! @{
    //--------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief Found a city spanning GridConfig::defaultCitySizeU by
    //! GridConfig::defaultCitySizeV cells. A city of the same name is replaced.
    //! \param[in] name unique name of the city.
    //! \param[in] position world position of the top-left corner of its cells.
    //! \return the new city.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Found a city spanning a given number of cells.
    //! \param[in] name unique name of the city.
    //! \param[in] position world position of the top-left corner of its cells.
    //! \param[in] sizeU how many cells it owns along U.
    //! \param[in] sizeV how many cells it owns along V.
    //! \return the new city.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name,
                  Vector3f const& position,
                  uint32_t sizeU,
                  uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief Destroy a city and everything it holds. The layers stay, being
    //! shared with the other cities.
    //! \param[in] name name of the city.
    //! \return false when no city goes by that name.
    // -------------------------------------------------------------------------
    bool removeCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Look a city up by name.
    //! \param[in] name name of the city.
    //! \return the city.
    //! \throw std::out_of_range when no city goes by that name.
    // -------------------------------------------------------------------------
    [[nodiscard]] City& getCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Look a city up by name, without throwing.
    //! \param[in] name name of the city.
    //! \return the city, or nullptr when no city goes by that name.
    // -------------------------------------------------------------------------
    [[nodiscard]] City* findCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Which city owns a place. The editor asks before building.
    //! \param[in] position the place, in world coordinates.
    //! \return the city whose cells hold it, or nullptr when it falls outside
    //! every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] City* findCityAt(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief \return every city, by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] Cities const& getCities() const
    {
        return m_world.getCities();
    }

    //! @}

    //--------------------------------------------------------------------------
    //! \name Grid and layers
    //! @{
    //--------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief Look a layer up by name.
    //! \param[in] name name of the layer, such as "Water".
    //! \return the layer, or nullptr when no city ever asked for it. Add one
    //! through City::addLayer().
    // -------------------------------------------------------------------------
    [[nodiscard]] Layer* findLayer(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief \return every layer, by name. Shared by every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layers const& getLayers() const
    {
        return m_world.getLayers();
    }

    // -------------------------------------------------------------------------
    //! \brief Which cell a place falls in.
    //!
    //! Nothing is clamped: coordinates are signed, the grid has no bounds, and
    //! every place falls in a cell even outside every city. City::worldToCell()
    //! clamps to the cells of one city instead.
    //!
    //! \param[in] position the place, in world coordinates.
    //! \return the cell.
    // -------------------------------------------------------------------------
    Cell worldToCell(Vector3f const& position) const;

    // -------------------------------------------------------------------------
    //! \brief Where a cell sits in the world.
    //! \param[in] cell the cell.
    //! \return the world position of its top-left corner.
    // -------------------------------------------------------------------------
    Vector3f cellToWorld(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \brief Build a road from one place to another, cut at every city border.
    //!
    //! The road is clipped to the cells of each city. A road crossing a border
    //! becomes two segments, one per city. A road outside every city goes
    //! entirely to the city that asked. Each neighbour is asked through the
    //! Listener before a segment is built inside it.
    //!
    //! \param[in] owner the city that builds the road.
    //! \param[in] pathType name of the network, such as "Road".
    //! \param[in] segmentType recipe of the segment.
    //! \param[in] from, to endpoints of the road, in world coordinates.
    //! \return false when a neighbour refused. Nothing is built at all.
    // -------------------------------------------------------------------------
    bool addRoad(City& owner,
                 std::string const& pathType,
                 SegmentType const& segmentType,
                 Vector3f const& from,
                 Vector3f const& to);

    //! @}

    // -------------------------------------------------------------------------
    //! \brief How far the traffic is from equilibrium.
    //!
    //! This costs a whole graph search per agent looked at, so it is a
    //! diagnostic rather than something the simulation itself calls. Two things
    //! keep it affordable when a panel reads it on every frame: the answer is
    //! memoized until the next tick, and at most
    //! TrafficConfig::relativeGapSamples agents are looked at.
    //!
    //! \return the three numbers, all zero when nobody is on the road.
    // -------------------------------------------------------------------------
    [[nodiscard]] TrafficMetrics getTrafficMetrics() const;

private:

    // -------------------------------------------------------------------------
    //! \brief Work the traffic metrics out again, unless they were already
    //! computed during this tick.
    // -------------------------------------------------------------------------
    void updateTrafficMetrics() const;

private:

    //! \brief The ruleset. Declared before the world on purpose: destruction
    //! runs in reverse, so the world goes first and its buildings stop
    //! referring to these recipes before they are destroyed.
    Ruleset m_ruleset;
    //! \brief Game calendar, advanced once per tick.
    SimulationClock m_clock;
    //! \brief The shared grid, the layers and the cities.
    World m_world;
    //! \brief Game time accumulated but not yet spent on a tick. This is what
    //! keeps the rate of the simulation independent of the frame rate.
    float m_time = 0.0f;
    //! \brief How much game time passes per second of wall time.
    float m_timeScale = 1.0f;
    //! \brief While true, update() does nothing at all.
    bool m_paused = true;
    //! \brief Last traffic metrics. Mutable because the memoization happens
    //! inside a const method.
    mutable TrafficMetrics m_trafficMetrics;
    //! \brief Tick the metrics above were computed at. The starting value is a
    //! tick no run ever reaches, so the first call always computes.
    mutable uint64_t m_trafficMetricsTick =
        std::numeric_limits<uint64_t>::max();
};

} // namespace ogb

#endif
