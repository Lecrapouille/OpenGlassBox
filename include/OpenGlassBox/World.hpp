//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file World.hpp
//! \brief The grid shared by every city: layers, cities, coordinates.

#ifndef OPEN_GLASSBOX_WORLD_HPP
#define OPEN_GLASSBOX_WORLD_HPP

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Listener.hpp"
#include "OpenGlassBox/SimulationClock.hpp"

namespace ogb
{

//==============================================================================
//! \brief The ground every city stands on: one grid, one layer per resource
//! type.
//!
//! The grid is unique. Layers such as pollution or land value belong to the
//! World, not to each city, so two neighbouring cities share the same layers
//! along their border. A city keeps its own name, roads and buildings, and owns
//! only its rectangle of cells. Its rules run inside that rectangle.
//!
//! World also fixes the order of a tick: cities first, so agents move and then
//! buildings update, and layers last, so a cell reflects the whole tick.
//!
//! A Simulation owns one and does not hand it out. Reach a city through
//! Simulation::getCity() rather than here.
//==============================================================================
class World
{
public:

    //! \brief Callbacks for cities appearing, going away, and roads crossing a
    //! border. See SimulationListener.
    using Listener = SimulationListener;

    // -------------------------------------------------------------------------
    //! \brief An empty world: no city, no layer.
    //! \param[in] config runtime settings, copied and shared with every city.
    //! \param[in] clock game calendar, owned by the Simulation. Advanced once
    //! per tick by update().
    // -------------------------------------------------------------------------
    World(Config const& config, SimulationClock& clock);

    World(World const&) = delete;
    World& operator=(World const&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Register the callbacks. One listener at a time: a second call
    //! replaces the first.
    //! \param[in] listener kept by address, has to outlive the World.
    // -------------------------------------------------------------------------
    void setListener(Listener& listener)
    {
        m_listener = &listener;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the registered callbacks, or the default ones, which
    //! accept every road.
    // -------------------------------------------------------------------------
    [[nodiscard]] Listener& getListener()
    {
        return *m_listener;
    }

    // -------------------------------------------------------------------------
    //! \brief Create a layer for a resource type, or return the existing one.
    //!
    //! Layers are shared: when city B asks for a layer city A already created,
    //! both use the same one.
    //!
    //! \param[in] type recipe of the layer, from the ruleset.
    //! \return the layer, owned by the World.
    // -------------------------------------------------------------------------
    Layer& addLayer(LayerType const& type);

    // -------------------------------------------------------------------------
    //! \brief Look a layer up by name.
    //! \param[in] name name of the layer, such as "Water".
    //! \return the layer, or nullptr when no city ever asked for it.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layer* findLayer(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief \return every layer, by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layers const& getLayers() const
    {
        return m_layers;
    }

    // -------------------------------------------------------------------------
    //! \brief Create a city over a rectangle of the grid. A city of the same
    //! name is replaced.
    //! \param[in] name unique name of the city.
    //! \param[in] position world position of the top-left corner of its cells.
    //! \param[in] sizeU cells it owns along U.
    //! \param[in] sizeV cells it owns along V.
    //! \return the new city.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name,
                  Vector3f const& position,
                  uint32_t sizeU,
                  uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief The same, spanning GridConfig::defaultCitySizeU by
    //! GridConfig::defaultCitySizeV cells.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Destroy a city and everything it holds. The layers stay, being
    //! shared.
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
        return m_cities;
    }

    // -------------------------------------------------------------------------
    //! \brief One tick for the whole world: every city first, then every layer.
    //!
    //! Agents move, then buildings update, then layers run their rules. The
    //! clock is advanced here too.
    //!
    //! \param[in] dt how long the tick lasts, in seconds of game time.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief \return the side of a grid cell, in world units. The same for
    //! every layer and every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCellSize() const
    {
        return m_config.grid.cellSize;
    }

    // -------------------------------------------------------------------------
    //! \brief Which cell a place falls in.
    //!
    //! Nothing is clamped: coordinates are signed, the grid has no bounds, and
    //! every place falls in a cell even outside every city.
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
    //! \brief \return the runtime settings, shared by every city.
    // -------------------------------------------------------------------------
    [[nodiscard]] Config const& getConfig() const
    {
        return m_config;
    }

    // -------------------------------------------------------------------------
    //! \brief Replace the runtime settings while the simulation runs.
    //! \param[in] config the new settings, copied.
    //! \note GridConfig::cellSize is read when a city is created and when it
    //! moves. Changing it under a city that already exists leaves its cells
    //! where they were.
    // -------------------------------------------------------------------------
    void setConfig(Config const& config);

    // -------------------------------------------------------------------------
    //! \brief \return the game calendar. The Simulation owns it and advances
    //! it.
    // -------------------------------------------------------------------------
    [[nodiscard]] SimulationClock const& getClock() const
    {
        return m_clock;
    }

    // -------------------------------------------------------------------------
    //! \brief Build a road from one place to another, cut at every city border.
    //!
    //! The road is clipped to the cells of each city. A road crossing a border
    //! becomes two segments, one per city. A road outside every city goes
    //! entirely to the city that asked. Each neighbour is asked through the
    //! Listener before a segment is built inside it.
    //!
    //! \param[in] owner the city that builds the road.
    //! \param[in] pathType name of the network, such as "Road". When the target
    //! city has no such network, one is created from the network of the
    //! requester. The segment is skipped when the requester has none either.
    //! \param[in] segmentType recipe of the segment.
    //! \param[in] from, to endpoints of the road, in world coordinates.
    //! \return false when a neighbour refused. Nothing is built at all.
    // -------------------------------------------------------------------------
    bool addRoad(City& owner,
                 std::string const& pathType,
                 SegmentType const& segmentType,
                 Vector3f const& from,
                 Vector3f const& to);

private:

    //! \brief Runtime settings shared by the whole world.
    Config m_config;
    //! \brief Game calendar, not owned. Advanced once per tick by update().
    SimulationClock& m_clock;
    //! \brief One layer per resource type, shared by every city. Declared
    //! before the cities so it outlives the buildings pointing at it.
    Layers m_layers;
    //! \brief Every city of the world.
    Cities m_cities;
    //! \brief The default callbacks, which accept every road.
    Listener m_defaultListener;
    //! \brief The registered callbacks. Not owned.
    Listener* m_listener = &m_defaultListener;
};

} // namespace ogb

#endif
