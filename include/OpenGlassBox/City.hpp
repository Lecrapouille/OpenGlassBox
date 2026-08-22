//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file City.hpp
//! \brief An administrative region: road network, buildings, agents and map ownership.


#ifndef OPEN_GLASSBOX_CITY_HPP
#  define OPEN_GLASSBOX_CITY_HPP

#  include "OpenGlassBox/Unit.hpp"
#  include "OpenGlassBox/Agent.hpp"
#  include "OpenGlassBox/Area.hpp"
#  include "OpenGlassBox/Map.hpp"
#  include "OpenGlassBox/MapRegion.hpp"
#  include "OpenGlassBox/Path.hpp"
#  include "OpenGlassBox/Dijkstra.hpp"
#  include "OpenGlassBox/Config.hpp"
#  include <memory>

namespace ogb {

class Path;
class Node;
class World;

//==============================================================================
//! \brief An administrative region of the World: a name, a budget, its roads,
//! its buildings and the agents travelling between them.
//!
//! The maps are not its own. They belong to the World and are shared with the
//! neighbouring cities; what a City owns on the grid is the region it
//! administers, which is what bounds the rules run on its behalf.
//==============================================================================
class City
{
public:

    //==========================================================================
    //! \brief
    //==========================================================================
    class Listener
    {
    public:

        virtual ~Listener() = default;

        virtual void onMapAdded(Map& /*map*/) {}
        virtual void onMapRemoved(Map& /*map*/) {}

        virtual void onPathAdded(Path& /*path*/) {}
        virtual void onPathRemoved(Path& /*path*/) {}

        virtual void onUnitAdded(Unit& /*unit*/) {}
        virtual void onUnitRemoved(Unit& /*unit*/) {}

        virtual void onAgentAdded(Agent& /*agent*/) {}
        virtual void onAgentRemoved(Agent& /*agent*/) {}

        virtual void onAreaAdded(Area& /*area*/) {}
        virtual void onAreaRemoved(Area& /*area*/) {}
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Found a city administering sizeU x sizeV cells of the world grid,
    //! starting at the given world position. Prefer World::addCity, which also
    //! registers the city in the world.
    // -------------------------------------------------------------------------
    City(std::string const& name, Vector3f position, uint32_t sizeU,
         uint32_t sizeV, World& world);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setListener(City::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief Move the agents and execute the rule scripts of the Units. The
    //! rules of the maps are run by the World, which owns them.
    //! \param[in] dt: the duration of the tick, in seconds of game time.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief Run a single tick lasting the duration defined by the
    //! SimulationConfig of this City.
    // -------------------------------------------------------------------------
    void update() { update(config().tickDuration()); }

    // -------------------------------------------------------------------------
    //! \brief Ask the World for the Map holding this resource type, creating it
    //! if this is the first city to need it. The Map is shared with the other
    //! cities.
    //! \return the Map instance owned by the World.
    // -------------------------------------------------------------------------
    Map& addMap(MapType const& type);

    // -------------------------------------------------------------------------
    //! \brief Get the reference of the Map or throw an exception if the Map
    //! does not exist.
    //! \param[in] name: the unique identifier of the Map.
    //! \throw std::exception if the Map is not found.
    //! \return the instance of the Map, owned by the World.
    // -------------------------------------------------------------------------
    Map& getMap(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Path. Destroy the Path of the same name if
    //! the identifier is already known.
    //! \param[in] type: the type of Path parsed from the simulation script.
    //! \return the newly created Path instance.
    // -------------------------------------------------------------------------
    Path& addPath(PathType const& type);

    // -------------------------------------------------------------------------
    //! \brief Get the reference of the Path or throw an exception if the Path
    //! does not exist.
    //! \param[in] name: the unique identifier of the Path.
    //! \throw std::exception if the Path is not found.
    //! \return the instance of the Path.
    // -------------------------------------------------------------------------
    Path& getPath(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Unit.
    //! \param[in] type: the type of Path parsed from the simulation script.
    //! \param[in] node: A Path node affected to the position of the Unit.
    //! \return the newly created Unit instance.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Attach a Unit to a Way at the given offset, without splitting
    //! the segment. This is how a street of houses stays a single Way.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Path& path, Way& way, float offset);

    // -------------------------------------------------------------------------
    //! \brief Place a Unit at a free world position, with no attachment to the
    //! road network.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Found an Area administering the given cells.
    // -------------------------------------------------------------------------
    Area& addArea(AreaType const& type, MapRegion const& footprint);

    // -------------------------------------------------------------------------
    Agent& addAgent(AgentType const& type, Unit& owner, Resources const& resources,
                    std::string const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief Destroy a Unit and detach it from the Node it sits on. The Node
    //! and the Path it belongs to are left untouched.
    // -------------------------------------------------------------------------
    void removeUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief Destroy an Area. The Units it spawned stay: they now belong to
    //! the City alone.
    // -------------------------------------------------------------------------
    void removeArea(Area& area);

    // -------------------------------------------------------------------------
    //! \brief Destroy a segment of a Path together with the Units sitting on
    //! it. Agents using the segment or waiting at either end are recycled:
    //! their cargo goes back to the owner Unit (or a compatible one) before
    //! they are removed.
    // -------------------------------------------------------------------------
    void removeWay(Path& path, Way& way);

    // -------------------------------------------------------------------------
    //! \brief Destroy a node of a Path, its incident segments and the Units on
    //! them. Agents using the node or those segments are recycled to their
    //! owner.
    // -------------------------------------------------------------------------
    void removeNode(Path& path, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Translate the position of the City inside the world coordinate.
    //! This also change the position of Path, Unit, Agent ... hold by the City.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

    // -------------------------------------------------------------------------
    //! \brief Get the cell index U and V from a given position inside the world
    //! coordinate, clamped to the region of this City.
    //! \param[in] worldPos: the world position.
    //! \param[out] u: the grid index along the U-axis.
    //! \param[out] v: the grid index along the V-axis.
    // -------------------------------------------------------------------------
    void world2mapPosition(Vector3f worldPos, int32_t& u, int32_t& v) const;

    // -------------------------------------------------------------------------
    //! \brief Return the name (unique identifier).
    // -------------------------------------------------------------------------
    std::string const& name() const { return m_name; }

    // -------------------------------------------------------------------------
    //! \brief Return the position inside the World coordinate of the city (top-left corner).
    // -------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief The cells of the world grid this City administers.
    // -------------------------------------------------------------------------
    MapRegion region() const;

    // -------------------------------------------------------------------------
    //! \brief The World this City belongs to, owner of the maps and of the grid.
    // -------------------------------------------------------------------------
    World& world() { return m_world; }
    World const& world() const { return m_world; }

    // -------------------------------------------------------------------------
    //! \brief Return the number of cells the region spans along the U-axis.
    // -------------------------------------------------------------------------
    uint32_t gridSizeU() const { return m_gridSizeU; }

    // -------------------------------------------------------------------------
    //! \brief Return the number of cells the region spans along the V-axis.
    // -------------------------------------------------------------------------
    uint32_t gridSizeV() const { return m_gridSizeV; }

    // -------------------------------------------------------------------------
    //! \brief Return the runtime settings, held by the World and shared with
    //! every other City. They can be tuned live: the tick duration and the
    //! traffic smoothing are read at every tick.
    // -------------------------------------------------------------------------
    SimulationConfig const& config() const;
    SimulationConfig& config();

    // -------------------------------------------------------------------------
    //! \brief Return the length of the side of a grid cell, in world units.
    // -------------------------------------------------------------------------
    float gridCellSize() const;

    // -------------------------------------------------------------------------
    //! \brief Return global resources.
    // -------------------------------------------------------------------------
    Resources& globals() { return m_globals; }
    Resources const& globals() const { return m_globals; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Maps of the World. They are shared with
    //! the other cities.
    // -------------------------------------------------------------------------
    Maps& maps();

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Paths.
    // -------------------------------------------------------------------------
    Paths& paths() { return m_paths; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Units.
    // -------------------------------------------------------------------------
    Units& units() { return m_units; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Agents.
    // -------------------------------------------------------------------------
    Agents& agents() { return m_agents; }

    Areas& areas() { return m_areas; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Maps of the World. They are shared with
    //! the other cities.
    // -------------------------------------------------------------------------
    Maps const& maps() const;

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Paths.
    // -------------------------------------------------------------------------
    Paths const& paths() const { return m_paths; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Units.
    // -------------------------------------------------------------------------
    Units const& units() const { return m_units; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Agents.
    // -------------------------------------------------------------------------
    Agents const& agents() const { return m_agents; }

    Areas const& areas() const { return m_areas; }

    IRouter& router() { return m_dijkstra; }
    IRouter const& router() const { return m_dijkstra; }

    // -------------------------------------------------------------------------
    //! \brief Empty the city: agents, units, areas, paths and globals. Maps
    //! of the World stay: they are shared. The ruleset is not touched.
    // -------------------------------------------------------------------------
    void clear();

    // -------------------------------------------------------------------------
    //! \brief After a Way was removed, drop Nodes that no longer have a Way.
    //! Agents standing on such a Node let go of it first.
    // -------------------------------------------------------------------------
    void removeOrphanNodes(Path& path);

    // -------------------------------------------------------------------------
    //! \brief Take away the Agents left with no road under them, which is what
    //! happens to all of them when the last Way of the City is demolished.
    // -------------------------------------------------------------------------
    void dropStrandedAgents();

private:

    //! \brief Name of the City, ie. "Paris", "Seattle", "NYC" ...
    std::string   m_name;
    //! \brief The World holding the grid and the maps.
    World&        m_world;
    //! \brief Position of the top-left corner.
    Vector3f      m_position;
    //! \brief The number of administered cells along the U-axis.
    uint32_t      m_gridSizeU;
    //! \brief The number of administered cells along the V-axis.
    uint32_t      m_gridSizeV;
    //! \brief Counter of Unit to create unique id.
    // FIXME Not used: uint32_t      m_nextUnitId = 0u;
    //! \brief Counter of Agent to create unique id.
    uint32_t      m_nextAgentId = 0u;
    uint32_t      m_nextUnitId = 0u;
    uint32_t      m_nextAreaId = 0u;
    //! \brief Globals resources (money, oil, electricity ...)
    Resources     m_globals;
    //! \brief Collection of graphs (roads, power lines, water pipes ...)
    Paths         m_paths;
    //! \brief Collection of building (house, factory ...)
    Units         m_units;
    //! \brief Collection of resource carrier (cars, citizens ...)
    Agents        m_agents;
    //! \brief Collection of painted zones.
    Areas         m_areas;
    //! \brief
    Dijkstra      m_dijkstra;
    //!
    City::Listener *m_listener;
};

} // namespace ogb

#endif
