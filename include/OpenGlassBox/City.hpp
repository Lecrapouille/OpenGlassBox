//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file City.hpp
//! \brief An administrative region: road network, buildings, agents and map
//! ownership.

#ifndef OPEN_GLASSBOX_CITY_HPP
#define OPEN_GLASSBOX_CITY_HPP

#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Area.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Dijkstra.hpp"
#include "OpenGlassBox/Map.hpp"
#include "OpenGlassBox/MapRegion.hpp"
#include "OpenGlassBox/Path.hpp"
#include "OpenGlassBox/Unit.hpp"
#include <memory>

namespace ogb
{

class Path;
class Node;
class World;

//==============================================================================
//! \brief One town: its roads, its buildings, the agents travelling between
//! them, and the zones growing new ones.
//!
//! A City is the thing that ticks. Every tick it moves its agents along the
//! roads, runs the rules of its buildings and of its zones, and lets the
//! traffic averages of its streets settle. It owns all of that, and hands out
//! references to it.
//!
//! What it does not own are the layers of the environment. Those belong to the
//! World and are shared with the neighbouring towns: pollution does not stop at
//! a boundary. What a City owns on the grid is the region it administers, which
//! is what bounds the rules run on its behalf, and which is why two towns may
//! read the same layer without treading on each other.
//!
//! Nothing here is thread safe, and nothing may be added or removed while a
//! tick is running: the demo queues the edits of the player and applies them
//! between two ticks.
//!
//! Example:
//! \code
//! Simulation simulation;
//! simulation.script().parse("simulations/city.ogs");
//!
//! City& city = simulation.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f), 32u,
//! 32u); Path& road = city.addPath(simulation.script().getPathType("Road"));
//! Node& a = road.addNode(Vector3f(0.0f, 0.0f, 0.0f));
//! Node& b = road.addNode(Vector3f(60.0f, 0.0f, 0.0f));
//! road.addWay(simulation.script().getWayType("Dirt"), a, b);
//!
//! city.addUnit(simulation.script().getUnitType("Home"), a);
//! city.addUnit(simulation.script().getUnitType("Work"), b);
//!
//! // One minute of game time.
//! for (uint32_t i = 0u; i < 1200u; ++i)
//!     city.update();
//!
//! std::cout << city.agents().size() << " agents on the road\n";
//! \endcode
//==============================================================================
class City
{
public:

    //==========================================================================
    //! \brief What the renderer subscribes to in order to keep its own picture
    //! of the city without walking it on every frame.
    //!
    //! Every method does nothing by default, so an implementation only
    //! overrides what it cares about. The reference handed to a callback is
    //! valid for the duration of the call only: a removal callback fires while
    //! the thing is still alive, and it is gone by the time the callback
    //! returns.
    //==========================================================================
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //! \brief A layer was made available to this city by the World.
        virtual void onMapAdded(Map& /*map*/) {}

        //! \brief A layer is about to go away.
        virtual void onMapRemoved(Map& /*map*/) {}

        //! \brief A network was created, empty of crossroads and segments.
        virtual void onPathAdded(Path& /*path*/) {}

        //! \brief A network is about to go away, with everything it holds.
        virtual void onPathRemoved(Path& /*path*/) {}

        //! \brief A building was put up, by the player or by a zone.
        virtual void onUnitAdded(Unit& /*unit*/) {}

        //! \brief A building is about to be demolished.
        virtual void onUnitRemoved(Unit& /*unit*/) {}

        //! \brief A building sent an agent out.
        virtual void onAgentAdded(Agent& /*agent*/) {}

        //! \brief An agent is about to go away, having arrived or been stranded
        //! by a demolished road.
        virtual void onAgentRemoved(Agent& /*agent*/) {}

        //! \brief A zone was painted.
        virtual void onAreaAdded(Area& /*area*/) {}

        //! \brief A zone is about to go away. The buildings it grew stay.
        virtual void onAreaRemoved(Area& /*area*/) {}
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Found a town administering a rectangle of the world grid.
    //!
    //! Prefer World::addCity or Simulation::addCity, which also register the
    //! town in the world: a City built directly is not known to the World, and
    //! the layers it asks for will not be run.
    //!
    //! \param[in] name unique name of the town, such as "Paris".
    //! \param[in] position world position of the top-left corner of its region.
    //! \param[in] sizeU how many cells it administers along U.
    //! \param[in] sizeV how many cells it administers along V.
    //! \param[in] world the world it belongs to. Kept by reference and has to
    //! outlive the City.
    // -------------------------------------------------------------------------
    City(std::string const& name,
         Vector3f position,
         uint32_t sizeU,
         uint32_t sizeV,
         World& world);

    // -------------------------------------------------------------------------
    //! \brief Subscribe to what happens in the town. Only one listener at a
    //! time: a second call replaces the first.
    //! \param[in] listener kept by address and has to outlive the City.
    // -------------------------------------------------------------------------
    void setListener(City::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief One tick: move the agents, run the rules of the buildings and of
    //! the zones, let the traffic averages settle.
    //!
    //! The rules of the layers are not run here. They belong to the World,
    //! which runs them once for every town.
    //!
    //! \param[in] dt how long the tick lasts, in seconds of game time. What
    //! turns the speed of an agent into a distance.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief One tick lasting whatever the settings say, which is what the
    //! game loop calls.
    // -------------------------------------------------------------------------
    void update()
    {
        update(config().tickDuration());
    }

    // -------------------------------------------------------------------------
    //! \brief Ask the World for a layer, which it creates when this is the
    //! first town to need it.
    //! \param[in] type recipe of the layer, from the ruleset.
    //! \return the layer, owned by the World and shared with the other towns.
    // -------------------------------------------------------------------------
    Map& addMap(MapType const& type);

    // -------------------------------------------------------------------------
    //! \brief \param[in] name name of the layer, such as "Water".
    //! \return the layer, owned by the World.
    //! \throw std::out_of_range when no such layer exists. Ask for it by name
    //! only when the ruleset is known to declare it.
    // -------------------------------------------------------------------------
    Map& getMap(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Create a network. An existing one of the same name is destroyed
    //! first, together with its crossroads and segments.
    //! \param[in] type recipe of the network, from the ruleset.
    //! \return the new, empty network.
    // -------------------------------------------------------------------------
    Path& addPath(PathType const& type);

    // -------------------------------------------------------------------------
    //! \brief \param[in] name name of the network, such as "Road".
    //! \return the network.
    //! \throw std::out_of_range when no such network exists.
    // -------------------------------------------------------------------------
    Path& getPath(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Put a building up on a crossroads, which becomes its address.
    //! \param[in] type recipe of the building, from the ruleset.
    //! \param[in] node the crossroads it stands on. An agent reaching that
    //! crossroads has reached the building.
    //! \return the new building.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Put a building up along a segment, without cutting it. This is
    //! how a street of forty houses stays one segment instead of becoming
    //! forty.
    //!
    //! An agent driving along the segment may hand its load over to such a
    //! building; one routing towards it aims at the nearer end of the segment.
    //!
    //! \param[in] type recipe of the building.
    //! \param[in] path the network the segment belongs to.
    //! \param[in] way the segment it stands along.
    //! \param[in] offset where along it, from 0 at way.from() to 1 at way.to().
    //! \return the new building.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Path& path, Way& way, float offset);

    // -------------------------------------------------------------------------
    //! \brief Put a building up in the middle of nowhere, attached to no road.
    //!
    //! Such a building runs its rules like any other, but no agent can reach it
    //! and the agents it sends out have nowhere to go. Used by the tests and by
    //! a zone that has not found a road yet.
    //!
    //! \param[in] type recipe of the building.
    //! \param[in] position where it stands, in world coordinates.
    //! \return the new building.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Paint a zone over a rectangle of cells.
    //! \param[in] type recipe of the zone, from the ruleset.
    //! \param[in] footprint the cells it covers, on the grid of the World.
    //! \return the new zone, which starts growing buildings on the next tick.
    // -------------------------------------------------------------------------
    Area& addArea(AreaType const& type, MapRegion const& footprint);

    // -------------------------------------------------------------------------
    //! \brief Send an agent out of a building, carrying something and looking
    //! for somewhere to put it. Called by the rules, and rarely by hand.
    //!
    //! \param[in] type recipe of the agent: how fast it drives, what colour it
    //! is drawn.
    //! \param[in] owner the building it leaves from, and where it stands to
    //! begin with.
    //! \param[in] resources what it carries. Copied: the load is the agent's
    //! own from now on.
    //! \param[in] searchTarget what it is looking for, which is the name of a
    //! kind of resource a building will take, such as "People".
    //! \return the new agent.
    // -------------------------------------------------------------------------
    Agent& addAgent(AgentType const& type,
                    Unit& owner,
                    Resources const& resources,
                    std::string const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief Demolish a building and take it out of the crossroads or the
    //! segment it stood on. The road itself is left alone.
    //!
    //! The agents it sent out keep going: they are looking for a kind of
    //! building, not for this one, and they will find another or be dropped for
    //! having nowhere to unload.
    //!
    //! \param[in] unit the building to demolish. Nothing may refer to it
    //! afterwards.
    // -------------------------------------------------------------------------
    void removeUnit(Unit& unit);

    // -------------------------------------------------------------------------
    //! \brief Unpaint a zone. The buildings it grew stay: they belong to the
    //! town now, and demolishing a zone is not the same as bulldozing it.
    //! \param[in] area the zone to unpaint.
    // -------------------------------------------------------------------------
    void removeArea(Area& area);

    // -------------------------------------------------------------------------
    //! \brief Demolish a segment, together with the buildings standing along
    //! it.
    //!
    //! The agents driving on it, or waiting at either end for it, are taken
    //! away rather than left addressing a road that is gone. What they carried
    //! goes back to the building that sent them out, or to another that will
    //! take it, so that demolishing a road does not quietly destroy resources.
    //!
    //! \param[in] path the network the segment belongs to.
    //! \param[in] way the segment to demolish.
    // -------------------------------------------------------------------------
    void removeWay(Path& path, Way& way);

    // -------------------------------------------------------------------------
    //! \brief Demolish a crossroads, the segments incident to it and the
    //! buildings standing on any of them.
    //!
    //! The agents concerned are recycled the same way as in removeWay().
    //!
    //! \param[in] path the network the crossroads belongs to.
    //! \param[in] node the crossroads to demolish.
    // -------------------------------------------------------------------------
    void removeNode(Path& path, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Cut a segment in two and give back the junction, which is a
    //! crossroads an agent may stop at and a building may stand on.
    //!
    //! Path::splitWay only rewires the graph. Here the town also puts things
    //! back where they belong: the buildings that stood along the segment are
    //! moved onto the half now running under them, and the agents driving on it
    //! let go and route again from where they are, so that none of them is left
    //! addressing a segment stopping short of it.
    //!
    //! This is what putting a building on a street does, which is why a street
    //! gains a crossroads every time a house is built on it.
    //!
    //! \param[in] path the network the segment belongs to.
    //! \param[in] way the segment to cut.
    //! \param[in] offset where to cut, from 0 at way.from() to 1 at way.to().
    //! \return the junction, or the end of the segment when the offset falls on
    //! one, in which case nothing was cut.
    // -------------------------------------------------------------------------
    Node& splitWay(Path& path, Way& way, float offset);

    // -------------------------------------------------------------------------
    //! \brief Move the town in the world, taking its roads, its buildings and
    //! its agents along. The cells it administers do not move: the region is
    //! recomputed from the new position.
    //! \param[in] direction how far to move it, in world units.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

    // -------------------------------------------------------------------------
    //! \brief Which cell of the grid a place falls in.
    //! \param[in] worldPos the place, in world coordinates.
    //! \param[out] u column of the cell.
    //! \param[out] v row of the cell.
    //! \note Clamped to the region of the town: a place outside it gives the
    //! nearest cell inside, not the cell it really falls in.
    // -------------------------------------------------------------------------
    void world2mapPosition(Vector3f worldPos, int32_t& u, int32_t& v) const;

    // -------------------------------------------------------------------------
    //! \brief \return the name of the town, unique in the World.
    // -------------------------------------------------------------------------
    std::string const& name() const
    {
        return m_name;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the world position of the top-left corner of its region.
    // -------------------------------------------------------------------------
    Vector3f const& position() const
    {
        return m_position;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the cells of the grid the town administers, computed from
    //! its position and its size. What bounds every rule run on its behalf.
    // -------------------------------------------------------------------------
    MapRegion region() const;

    // -------------------------------------------------------------------------
    //! \brief \return the world the town belongs to, owner of the layers and of
    //! the grid.
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
    //! \brief \return how many cells the region spans along U.
    // -------------------------------------------------------------------------
    uint32_t gridSizeU() const
    {
        return m_gridSizeU;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many cells the region spans along V.
    // -------------------------------------------------------------------------
    uint32_t gridSizeV() const
    {
        return m_gridSizeV;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the runtime settings, held by the World and shared with
    //! every other town. They may be changed while the game runs: the duration
    //! of a tick and the traffic smoothing are read on every tick.
    // -------------------------------------------------------------------------
    SimulationConfig const& config() const;

    //! \copydoc config() const
    SimulationConfig& config();

    // -------------------------------------------------------------------------
    //! \brief \return the side of a grid cell, in world units. Comes from the
    //! World, so every town of a world shares the same grid.
    // -------------------------------------------------------------------------
    float gridCellSize() const;

    // -------------------------------------------------------------------------
    //! \brief \return the resources belonging to the town as a whole rather
    //! than to a building: money, oil, electricity. What a \c global rule reads
    //! and writes.
    // -------------------------------------------------------------------------
    Resources& globals()
    {
        return m_globals;
    }

    //! \copydoc globals()
    Resources const& globals() const
    {
        return m_globals;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the layers of the World, by name. Shared with the other
    //! towns.
    // -------------------------------------------------------------------------
    Maps& maps();

    //! \copydoc maps()
    Maps const& maps() const;

    // -------------------------------------------------------------------------
    //! \brief \return the networks of the town, by name.
    // -------------------------------------------------------------------------
    Paths& paths()
    {
        return m_paths;
    }

    //! \copydoc paths()
    Paths const& paths() const
    {
        return m_paths;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the buildings of the town, in creation order.
    // -------------------------------------------------------------------------
    Units& units()
    {
        return m_units;
    }

    //! \copydoc units()
    Units const& units() const
    {
        return m_units;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the agents on the road, in creation order. The list turns
    //! over quickly: an agent lives from the building that sent it out to the
    //! one that takes its load.
    // -------------------------------------------------------------------------
    Agents& agents()
    {
        return m_agents;
    }

    //! \copydoc agents()
    Agents const& agents() const
    {
        return m_agents;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the zones painted on the town, in creation order.
    // -------------------------------------------------------------------------
    Areas& areas()
    {
        return m_areas;
    }

    //! \copydoc areas()
    Areas const& areas() const
    {
        return m_areas;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the router the agents ask for an itinerary. One per town,
    //! so that the memory it needs is allocated once instead of on every
    //! search.
    // -------------------------------------------------------------------------
    IRouter& router()
    {
        return m_dijkstra;
    }

    //! \copydoc router()
    IRouter const& router() const
    {
        return m_dijkstra;
    }

    // -------------------------------------------------------------------------
    //! \brief Empty the town: agents, buildings, zones, networks and globals.
    //!
    //! The layers of the World stay, being shared, and so does the ruleset:
    //! what the player drew goes away, what the script declared remains, so the
    //! town can be rebuilt without loading anything again.
    // -------------------------------------------------------------------------
    void clear();

    // -------------------------------------------------------------------------
    //! \brief Sweep away the crossroads left with no segment at all, which is
    //! what demolishing a street leaves behind. The agents standing on such a
    //! crossroads let go of it first.
    //! \param[in] path the network to sweep.
    // -------------------------------------------------------------------------
    void removeOrphanNodes(Path& path);

    // -------------------------------------------------------------------------
    //! \brief Take away the agents left with no road under them, which is what
    //! happens to every one of them when the last street of the town is
    //! demolished. Their load is handed back the same way as in removeWay().
    // -------------------------------------------------------------------------
    void dropStrandedAgents();

private:

    //! \brief Name of the town, unique in the World: "Paris", "Seattle", "NYC".
    std::string m_name;
    //! \brief The world holding the grid and the layers. Not owned.
    World& m_world;
    //! \brief World position of the top-left corner of the region.
    Vector3f m_position;
    //! \brief How many administered cells along U.
    uint32_t m_gridSizeU;
    //! \brief How many administered cells along V.
    uint32_t m_gridSizeV;
    //! \brief Identifier the next agent will be given. Never reused, so that a
    //! stale identifier names nothing rather than something else.
    uint32_t m_nextAgentId = 0u;
    //! \brief Identifier the next building will be given.
    uint32_t m_nextUnitId = 0u;
    //! \brief Identifier the next zone will be given.
    uint32_t m_nextAreaId = 0u;
    //! \brief Resources belonging to the town as a whole: money, oil,
    //! electricity.
    Resources m_globals;
    //! \brief The networks, owned: roads, power lines, water pipes.
    Paths m_paths;
    //! \brief The buildings, owned: houses, factories, shops.
    Units m_units;
    //! \brief The agents on the road, owned: cars, citizens, trucks.
    Agents m_agents;
    //! \brief The zones painted by the player, owned.
    Areas m_areas;
    //! \brief The router, one per town so that its scratch memory is allocated
    //! once rather than on every search.
    Dijkstra m_dijkstra;
    //! \brief Who to tell when something is added or removed, or nullptr when
    //! nobody is listening. Not owned.
    City::Listener* m_listener;
};

} // namespace ogb

#endif
