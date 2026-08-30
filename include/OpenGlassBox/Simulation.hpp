//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Simulation.hpp
//! \brief Entry point: ruleset, cities, time control.

#ifndef OPEN_GLASSBOX_SIMULATION_HPP
#define OPEN_GLASSBOX_SIMULATION_HPP

#include "OpenGlassBox/Ruleset.hpp"
#include "OpenGlassBox/SimulationClock.hpp"
#include "OpenGlassBox/World.hpp"

#include <limits>

namespace ogb
{

//==============================================================================
//! \brief The full game: ruleset, world, cities, and clock.
//!
//! Start here. This is the main class for applications. Reach layers, cities,
//! the grid, and the calendar from here. The World stays internal.
//!
//! The ruleset is declared before the World. The ruleset outlives all
//! references to it. A building holds a reference to its recipe. The recipe
//! lives in the ruleset.
//!
//! update() turns real time into game time. Pass seconds since the last frame.
//! It scales time by getTimeScale(), accumulates it, and runs fixed ticks.
//! The simulation runs at the same rate at any frame rate. A slow machine
//! falls behind. It does not simulate differently.
//!
//! Cities are not connected yet. An agent never leaves its home city. Cities
//! share the grid and the environment layers.
//!
//! All writes go through Simulation. Reads may use nested objects. Build a road
//! with addRoad(). Read cities with getCity(). Load a script with
//! loadScriptFile(). Read the ruleset with getRuleset().
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
//! // Game loop: real seconds in, fixed ticks out.
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

    //! \brief Callbacks when cities appear, disappear, or roads cross a border.
    //! City events use City::Listener.
    using Listener = SimulationListener;

    //==========================================================================
    //! \brief How far traffic is from equilibrium. Measured on a sample of
    //! agents.
    //!
    //! Compute all three values together and cache them until the next tick.
    //! A panel can read them every frame.
    //==========================================================================
    struct TrafficMetrics
    {
        //! \brief Relative gap between the current assignment and the cheapest
        //! routes at current travel times:
        //!
        //!     (totalTravelTime - shortestPathTime) / totalTravelTime
        //!
        //! Zero means every agent uses a cheapest route. This is Wardrop
        //! equilibrium. A large value means traffic is still settling. Zero
        //! when nobody is on the road. From Relgap in CiudadSim successive
        //! averages.
        float relativeGap = 0.0f;
        //! \brief Total time sampled agents spend travelling (TSTT).
        float totalTravelTime = 0.0f;
        //! \brief Time the same agents would spend on cheapest routes (SPTT).
        float shortestPathTime = 0.0f;
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Create a game with an empty ruleset and no city.
    //! \param[in] config Runtime settings. Copied and shared with every city.
    //! Defaults to ogb::Config.
    // -------------------------------------------------------------------------
    explicit Simulation(Config const& config = {});

    Simulation(Simulation const&) = delete;
    Simulation& operator=(Simulation const&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Register callbacks. Only one listener at a time. A new call
    //! replaces the old one.
    //!
    //! Register before you add the first city. Otherwise you miss its callback.
    //!
    //! \param[in] listener Stored by address. Must outlive the Simulation.
    // -------------------------------------------------------------------------
    void setListener(Listener& listener);

    //--------------------------------------------------------------------------
    //! \name Ruleset
    //! @{
    //--------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    //! \brief Load a script from a file. Replaces the previous ruleset.
    //!
    //! On failure, keep the previous ruleset. A bad reload does not empty a
    //! running game.
    //!
    //! \param[in] filename Script file to read. Pick the language from the
    //! extension.
    //! \return true on success. See getScriptErrors() for errors.
    //!
    //! \note Existing cities hold references into the old ruleset. Load the
    //! script before you add cities.
    // -------------------------------------------------------------------------
    bool loadScriptFile(std::string const& filename);

    // -------------------------------------------------------------------------
    //! \brief Load a script from memory. Used by tests and editors without a
    //! file.
    //! \param[in] source The script text.
    //! \param[in] name Name for error messages. No file path exists.
    //! \return true on success.
    // -------------------------------------------------------------------------
    bool loadScriptString(std::string const& source,
                          std::string const& name = "<string>");

    // -------------------------------------------------------------------------
    //! \return All errors from the last load, in order. Empty after
    //! success.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<ParseError> const& getScriptErrors() const
    {
        return m_ruleset.getErrors();
    }

    // -------------------------------------------------------------------------
    //! \return Last load errors, one per line, ready to display.
    // -------------------------------------------------------------------------
    std::string formatScriptErrors() const
    {
        return m_ruleset.formatErrors();
    }

    // -------------------------------------------------------------------------
    //! \return What the script declared: recipes for building cities.
    //! Read only. Cities hold references into it.
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
    //! \brief Advance the game to match real time.
    //!
    //! Scale elapsed time by getTimeScale(). Add leftover time from the last
    //! call. Spend time one fixed tick at a time. Keep unused time for the next
    //! call. Do not lose or invent game time. Does nothing while paused.
    //!
    //! \param[in] deltaTime Seconds since the last call.
    // -------------------------------------------------------------------------
    void update(float deltaTime); // TODO: use std::chrono::ms

    // -------------------------------------------------------------------------
    //! \brief Run one tick. Ignore leftover time, pause, and time scale. The
    //! debugger step button calls this.
    // -------------------------------------------------------------------------
    void stepOneTick();

    // -------------------------------------------------------------------------
    //! \return Game time per second of wall time. One is real time. Two
    //! is twice as fast. Zero freezes time without pausing.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getTimeScale() const
    {
        return m_timeScale;
    }

    // -------------------------------------------------------------------------
    //! \param[in] scale New time multiplier. Treat negative values as
    //! zero. Time does not run backwards.
    // -------------------------------------------------------------------------
    void setTimeScale(float scale)
    {
        m_timeScale = (scale < 0.0f) ? 0.0f : scale;
    }

    // -------------------------------------------------------------------------
    //! \return true while update() skips time and ticks.
    //! Keep leftover time. Unpause resumes instead of restarting.
    //!
    //! \note A new Simulation starts paused. Build cities before anything moves.
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
    //! \return Game calendar: day, hour, minute, and tick count. Read
    //! only. Use setTicks() or setTimeOfDay() to change it.
    // -------------------------------------------------------------------------
    [[nodiscard]] SimulationClock const& getClock() const
    {
        return m_clock;
    }

    // -------------------------------------------------------------------------
    //! \brief Restore the tick counter. Use when loading a save.
    //! \param[in] ticks Tick counter to restore.
    // -------------------------------------------------------------------------
    void setTicks(uint64_t ticks)
    {
        m_clock.setTicks(ticks);
    }

    // -------------------------------------------------------------------------
    //! \brief Set the date. Called at construction with TimeConfig::startHour.
    //! \param[in] day Days since zero.
    //! \param[in] hour Hour of the day, in [0..23].
    //! \param[in] minute Minute of the hour, in [0..59].
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
    //! \return Runtime settings, shared with every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] Config const& getConfig() const
    {
        return m_world.getConfig();
    }

    // -------------------------------------------------------------------------
    //! \brief Replace runtime settings while the game runs. Read current
    //! settings, change what you need, and pass them back.
    //!
    //! \param[in] config New settings. Copied.
    //! \note GridConfig::cellSize applies when a city is created or moved.
    //! Changing it under an existing city does not move its cells.
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
    //! \brief Add a city with default size GridConfig::defaultCitySizeU by
    //! GridConfig::defaultCitySizeV cells. Replace a city with the same name.
    //! \param[in] name Unique city name.
    //! \param[in] position World position of the top-left cell corner.
    //! \return The new city.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Add a city with a given cell size.
    //! \param[in] name Unique city name.
    //! \param[in] position World position of the top-left cell corner.
    //! \param[in] sizeU Cell count along U.
    //! \param[in] sizeV Cell count along V.
    //! \return The new city.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name,
                  Vector3f const& position,
                  uint32_t sizeU,
                  uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief Remove a city and everything it holds. Keep shared layers.
    //! \param[in] name City name.
    //! \return false when no city has that name.
    // -------------------------------------------------------------------------
    bool removeCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Find a city by name.
    //! \param[in] name City name.
    //! \return The city.
    //! \throw std::out_of_range when the name is missing.
    // -------------------------------------------------------------------------
    [[nodiscard]] City& getCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Find a city by name. Do not throw.
    //! \param[in] name City name.
    //! \return The city, or nullptr when the name is missing.
    // -------------------------------------------------------------------------
    [[nodiscard]] City* findCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Find which city owns a position. The editor uses this before
    //! building.
    //! \param[in] position Position in world coordinates.
    //! \return The city that contains it, or nullptr outside all cities.
    // -------------------------------------------------------------------------
    [[nodiscard]] City* findCityAt(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \return All cities, by name.
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
    //! \brief Find a layer by name.
    //! \param[in] name Layer name, such as "Water".
    //! \return The layer, or nullptr if no city created it. Add one with
    //! City::addLayer().
    // -------------------------------------------------------------------------
    [[nodiscard]] Layer* findLayer(std::string const& name);

    // -------------------------------------------------------------------------
    //! \return All layers, by name. Shared by every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layers const& getLayers() const
    {
        return m_world.getLayers();
    }

    // -------------------------------------------------------------------------
    //! \brief Find which cell contains a position.
    //!
    //! Do not clamp. Coordinates are signed. The grid has no bounds. Every
    //! position maps to a cell, even outside all cities. City::worldToCell()
    //! clamps to one city instead.
    //!
    //! \param[in] position Position in world coordinates.
    //! \return The cell.
    // -------------------------------------------------------------------------
    Cell worldToCell(Vector3f const& position) const;

    // -------------------------------------------------------------------------
    //! \brief Get the world position of a cell.
    //! \param[in] cell The cell.
    //! \return World position of its top-left corner.
    // -------------------------------------------------------------------------
    Vector3f cellToWorld(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \brief Build a road from one point to another. Cut it at every city
    //! border.
    //!
    //! Clip the road to each city's cells. A road across a border becomes two
    //! segments, one per city. A road outside all cities goes entirely to the
    //! city that requested it. Ask each neighbor through the Listener before
    //! you build inside it.
    //!
    //! \param[in] owner City that builds the road.
    //! \param[in] pathType Network name, such as "Road".
    //! \param[in] segmentType Segment recipe.
    //! \param[in] from, to Road endpoints in world coordinates.
    //! \return false when a neighbor refuses. Build nothing.
    // -------------------------------------------------------------------------
    bool addRoad(City& owner,
                 std::string const& pathType,
                 SegmentType const& segmentType,
                 Vector3f const& from,
                 Vector3f const& to);

    //! @}

    // -------------------------------------------------------------------------
    //! \brief Measure how far traffic is from equilibrium.
    //!
    //! This runs a full graph search per sampled agent. Use it as a diagnostic,
    //! not inside the simulation loop. Two things keep it fast when a panel reads
    //! it every frame: cache the result until the next tick, and sample at most
    //! TrafficConfig::relativeGapSamples agents.
    //!
    //! \return All three values. All zero when nobody is on the road.
    // -------------------------------------------------------------------------
    [[nodiscard]] TrafficMetrics getTrafficMetrics() const;

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute traffic metrics unless this tick already did.
    // -------------------------------------------------------------------------
    void updateTrafficMetrics() const;

    // -------------------------------------------------------------------------
    //! \brief Add the travel times of the agents of one city to the two running
    //! sums the relative gap is made of.
    //!
    //! Only part of the population is looked at when the city is a large one,
    //! one agent in every stride. That samples the whole population instead of
    //! whichever end of the list the loop starts at, and since both sums are
    //! scaled the same way, their ratio needs no correction.
    //!
    //! \param[in] city the city to walk.
    //! \param[in] budget how many agents may be sampled, zero for all of them.
    //! \param[in,out] tstt what the agents will actually spend on the road.
    //! \param[in,out] sptt what they would spend on the shortest way there.
    // -------------------------------------------------------------------------
    static void sampleCityTravelTimes(City const& city,
                                      uint32_t budget,
                                      float& tstt,
                                      float& sptt);

private:

    //! \brief The ruleset. Declared before the world on purpose. Destruction
    //! runs in reverse. The world is destroyed first. Buildings stop referring
    //! to these recipes before the ruleset is destroyed.
    Ruleset m_ruleset;
    //! \brief Game calendar. Advance once per tick.
    SimulationClock m_clock;
    //! \brief Shared grid, layers, and cities.
    World m_world;
    //! \brief Game time not yet spent on a tick. Keeps simulation rate
    //! independent of frame rate.
    float m_time = 0.0f;
    //! \brief Game time per second of wall time.
    float m_timeScale = 1.0f;
    //! \brief Tick when the metrics below were computed. Start with a tick no
    //! run reaches, so the first call always computes.
    mutable uint64_t m_trafficMetricsTick =
        std::numeric_limits<uint64_t>::max();
    //! \brief Last traffic metrics. Mutable because caching happens in a const
    //! method.
    mutable TrafficMetrics m_trafficMetrics;
    //! \brief When true, update() does nothing. Declared last, with the three
    //! floats above, so that it does not open a gap of its own.
    bool m_paused = true;
};

} // namespace ogb

#endif
