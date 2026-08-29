//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file World.hpp
//! \brief Grid for all cities: layers, cities, coordinates.

#ifndef OPEN_GLASSBOX_WORLD_HPP
#define OPEN_GLASSBOX_WORLD_HPP

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Listener.hpp"
#include "OpenGlassBox/SimulationClock.hpp"

namespace ogb
{

//==============================================================================
//! \brief Base grid for every city: one grid, one layer per resource type.
//!
//! The grid is shared. Layers like pollution or land value live on the World,
//! not in each city. Neighbouring cities share layers at their border. Each city
//! has its own name, roads, and buildings. It owns one rectangle of cells. Its
//! rules run inside that rectangle.
//!
//! World sets tick order: cities first, then layers. Agents move and buildings
//! update in cities first. Layers run last so each cell sees the full tick.
//!
//! Simulation owns one World. Do not access cities here. Use
//! Simulation::getCity() instead.
//==============================================================================
class World
{
public:

    //! \brief Callbacks for city creation, removal, and cross-border roads. See
    //! SimulationListener.
    using Listener = SimulationListener;

    // -------------------------------------------------------------------------
    //! \brief Create an empty world with no city and no layer.
    //! \param[in] config Runtime settings. Copied and shared with every city.
    //! \param[in] clock Game calendar. Owned by Simulation. update() advances
    //! it once per tick.
    // -------------------------------------------------------------------------
    World(Config const& config, SimulationClock& clock);

    World(World const&) = delete;
    World& operator=(World const&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Register callbacks. Only one listener is active. A new call
    //! replaces the old one.
    //! \param[in] listener Stored by address. Must outlive the World.
    // -------------------------------------------------------------------------
    void setListener(Listener& listener)
    {
        m_listener = &listener;
    }

    // -------------------------------------------------------------------------
    //! \return Registered callbacks, or default callbacks that accept
    //! every road.
    // -------------------------------------------------------------------------
    [[nodiscard]] Listener& getListener()
    {
        return *m_listener;
    }

    // -------------------------------------------------------------------------
    //! \brief Create a layer for a resource type, or return the existing layer.
    //!
    //! Layers are shared. If city B requests a layer city A already created,
    //! both use it.
    //!
    //! \param[in] type Layer recipe from the ruleset.
    //! \return The layer. Owned by the World.
    // -------------------------------------------------------------------------
    Layer& addLayer(LayerType const& type);

    // -------------------------------------------------------------------------
    //! \brief Find a layer by name.
    //! \param[in] name Layer name, such as "Water".
    //! \return The layer, or nullptr if no city created it.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layer* findLayer(std::string const& name);

    // -------------------------------------------------------------------------
    //! \return All layers, keyed by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layers const& getLayers() const
    {
        return m_layers;
    }

    // -------------------------------------------------------------------------
    //! \brief Create a city over a grid rectangle. Replaces a city with the
    //! same name.
    //! \param[in] name Unique city name.
    //! \param[in] position World position of the top-left cell corner.
    //! \param[in] sizeU Number of cells along U.
    //! \param[in] sizeV Number of cells along V.
    //! \return The new city.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name,
                  Vector3f const& position,
                  uint32_t sizeU,
                  uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief Same as addCity(), but uses GridConfig::defaultCitySizeU by
    //! GridConfig::defaultCitySizeV cells.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Destroy a city and everything it owns. Shared layers remain.
    //! \param[in] name City name.
    //! \return false if no city has that name.
    // -------------------------------------------------------------------------
    bool removeCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Find a city by name.
    //! \param[in] name City name.
    //! \return The city.
    //! \throw std::out_of_range if no city has that name.
    // -------------------------------------------------------------------------
    [[nodiscard]] City& getCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Find a city by name without throwing.
    //! \param[in] name City name.
    //! \return The city, or nullptr if no city has that name.
    // -------------------------------------------------------------------------
    [[nodiscard]] City* findCity(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Find the city that owns a position. The editor uses this before
    //! building.
    //! \param[in] position Position in world coordinates.
    //! \return The city that contains it, or nullptr if it is outside every
    //! city.
    // -------------------------------------------------------------------------
    [[nodiscard]] City* findCityAt(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \return All cities, keyed by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] Cities const& getCities() const
    {
        return m_cities;
    }

    // -------------------------------------------------------------------------
    //! \brief Run one tick for the whole world. Updates every city, then every
    //! layer.
    //!
    //! Agents move and buildings update in cities first. Layers run their rules
    //! last. The clock advances here.
    //!
    //! \param[in] dt Tick length in seconds of game time.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \return Grid cell side length in world units. Same for every
    //! layer and city.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCellSize() const
    {
        return m_config.grid.cellSize;
    }

    // -------------------------------------------------------------------------
    //! \brief Convert a world position to a cell.
    //!
    //! Coordinates are not clamped. The grid has no bounds. Every position maps
    //! to a cell, even outside every city.
    //!
    //! \param[in] position Position in world coordinates.
    //! \return The cell.
    // -------------------------------------------------------------------------
    Cell worldToCell(Vector3f const& position) const;

    // -------------------------------------------------------------------------
    //! \brief Convert a cell to world coordinates.
    //! \param[in] cell The cell.
    //! \return World position of its top-left corner.
    // -------------------------------------------------------------------------
    Vector3f cellToWorld(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \return Runtime settings shared by every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] Config const& getConfig() const
    {
        return m_config;
    }

    // -------------------------------------------------------------------------
    //! \brief Replace runtime settings during simulation.
    //! \param[in] config New settings. Copied.
    //! \note GridConfig::cellSize is read when a city is created or moved.
    //! Changing it after a city exists does not move its cells.
    // -------------------------------------------------------------------------
    void setConfig(Config const& config);

    // -------------------------------------------------------------------------
    //! \return Game calendar. Owned by Simulation. update() advances it.
    // -------------------------------------------------------------------------
    [[nodiscard]] SimulationClock const& getClock() const
    {
        return m_clock;
    }

    // -------------------------------------------------------------------------
    //! \brief Build a road between two points. Split it at every city border.
    //!
    //! The road is clipped to each city's cells. A road that crosses a border
    //! becomes two segments, one per city. A road outside every city goes to
    //! the requesting city only. The Listener asks each neighbour before
    //! building inside it.
    //!
    //! \param[in] owner City that builds the road.
    //! \param[in] pathType Network name, such as "Road". If the target city has
    //! no such network, one is created from the requester's network. Skips the
    //! segment if the requester has no network either.
    //! \param[in] segmentType Segment recipe.
    //! \param[in] from, to Road endpoints in world coordinates.
    //! \return false if a neighbour refused. Nothing is built.
    // -------------------------------------------------------------------------
    bool addRoad(City& owner,
                 std::string const& pathType,
                 SegmentType const& segmentType,
                 Vector3f const& from,
                 Vector3f const& to);

private:

    //! \brief Runtime settings shared by the whole world.
    Config m_config;
    //! \brief Game calendar. Not owned. update() advances it once per tick.
    SimulationClock& m_clock;
    //! \brief One layer per resource type, shared by every city. Declared
    //! before cities so buildings can reference it safely.
    Layers m_layers;
    //! \brief All cities in the world.
    Cities m_cities;
    //! \brief Default callbacks that accept every road.
    Listener m_defaultListener;
    //! \brief Registered callbacks. Not owned.
    Listener* m_listener = &m_defaultListener;
};

} // namespace ogb

#endif
