//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file City.hpp
//! \brief One city: roads, buildings, agents and zones.

#ifndef OPEN_GLASSBOX_CITY_HPP
#define OPEN_GLASSBOX_CITY_HPP

#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/Layer.hpp"
#include "OpenGlassBox/CellRegion.hpp"
#include "OpenGlassBox/Router.hpp"
#include "OpenGlassBox/Unit.hpp"

namespace ogb
{

class Path;
class Node;
class World;
class SimulationClock;

//==============================================================================
//! \brief One city: its roads, its buildings, the agents travelling between
//! them, and the zones growing new ones.
//!
//! A City is the thing that ticks. Every tick it moves its agents along the
//! roads, runs the rules of its buildings and of its zones, and lets the
//! traffic averages of its streets settle. It owns all of that and hands out
//! references to it.
//!
//! It does not own the layers of the environment. Those belong to the World and
//! are shared with the neighbouring cities: pollution does not stop at a
//! border. What a city owns on the grid is a rectangle of cells, and that
//! rectangle bounds every rule run on its behalf. This is why two cities can
//! read the same layer without treading on each other.
//!
//! Nothing here is thread safe, and nothing may be added or removed while a
//! tick is running: the demo queues the edits of the player and applies them
//! between two ticks.
//!
//! Example:
//! \code
//! ogb::Simulation simulation;
//! simulation.loadScriptFile("simulations/city.ogs");
//! ogb::Ruleset const& rules = simulation.getRuleset();
//!
//! ogb::City& city = simulation.addCity("Paris", { 0.0f, 0.0f, 0.0f });
//! ogb::Path& road = city.addPath(rules.getPathType("Road"));
//! ogb::Node& a = road.addNode({ 0.0f, 0.0f, 0.0f });
//! ogb::Node& b = road.addNode({ 60.0f, 0.0f, 0.0f });
//! road.addSegment(rules.getSegmentType("Dirt"), a, b);
//!
//! city.addUnit(rules.getUnitType("Home"), a);
//! city.addUnit(rules.getUnitType("Work"), b);
//!
//! // One minute of game time.
//! for (uint32_t i = 0u; i < 1200u; ++i)
//!     city.update();
//!
//! std::cout << city.getAgents().size() << " agents on the road\n";
//! \endcode
//==============================================================================
class City
{
public:

    //==========================================================================
    //! \brief Callbacks telling a renderer what changed inside the city, so
    //! that it does not have to walk the whole city on every frame.
    //!
    //! Every method does nothing by default, so an implementation overrides
    //! only what it cares about. The reference handed to a callback is valid
    //! for the duration of the call only: a removal callback fires while the
    //! thing is still alive, and it is gone by the time the callback returns.
    //==========================================================================
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //! \brief A layer was made available to this city by the World.
        virtual void onLayerAdded(Layer& /*layer*/) {}

        //! \brief A layer is about to go away.
        virtual void onLayerRemoved(Layer& /*layer*/) {}

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
        virtual void onZoneAdded(Zone& /*zone*/) {}

        //! \brief A zone is about to go away. The buildings it grew stay.
        virtual void onZoneRemoved(Zone& /*zone*/) {}
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Found a city over a rectangle of the world grid.
    //!
    //! Prefer Simulation::addCity(), which also registers the city in the
    //! world. A City built directly is not known to the World, and the layers
    //! it asks for will not be run.
    //!
    //! \param[in] name unique name of the city, such as "Paris".
    //! \param[in] position world position of the top-left corner of its cells.
    //! \param[in] sizeU how many cells it owns along U.
    //! \param[in] sizeV how many cells it owns along V.
    //! \param[in] world the world it belongs to. Kept by reference and has to
    //! outlive the City.
    // -------------------------------------------------------------------------
    City(std::string const& name,
         Vector3f const& position,
         uint32_t sizeU,
         uint32_t sizeV,
         World& world);

    City(City const&) = delete;
    City& operator=(City const&) = delete;
    City(City&&) = delete;
    City& operator=(City&&) = delete;

    // -------------------------------------------------------------------------
    //! \brief Register the callbacks. One listener at a time: a second call
    //! replaces the first.
    //! \param[in] listener kept by address, has to outlive the City.
    // -------------------------------------------------------------------------
    void setListener(City::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief One tick: move the agents, run the rules of the buildings and of
    //! the zones, let the traffic averages settle.
    //!
    //! The rules of the layers are not run here. They belong to the World,
    //! which runs them once for every city.
    //!
    //! \param[in] dt how long the tick lasts, in seconds of game time. This is
    //! what turns the speed of an agent into a distance.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief One tick lasting whatever the settings say, which is what the
    //! game loop calls.
    // -------------------------------------------------------------------------
    void update();

    // -------------------------------------------------------------------------
    //! \brief Ask the World for a layer, which it creates when this is the
    //! first city to need it.
    //! \param[in] type recipe of the layer, from the ruleset.
    //! \return the layer, owned by the World and shared with the other cities.
    // -------------------------------------------------------------------------
    Layer& addLayer(LayerType const& type);

    // -------------------------------------------------------------------------
    //! \brief Look a layer up by name.
    //! \param[in] name name of the layer, such as "Water".
    //! \return the layer.
    //! \throw std::out_of_range when no city ever asked for that layer.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layer& getLayer(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Create a network. An existing one of the same name is destroyed
    //! first, together with its crossroads and segments.
    //! \param[in] type recipe of the network, from the ruleset.
    //! \return the new, empty network.
    // -------------------------------------------------------------------------
    Path& addPath(PathType const& type);

    // -------------------------------------------------------------------------
    //! \brief Look a network up by name.
    //! \param[in] name name of the network, such as "Road".
    //! \return the network.
    //! \throw std::out_of_range when the city has no such network.
    // -------------------------------------------------------------------------
    [[nodiscard]] Path& getPath(std::string const& name);

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
    //! \param[in] segment the segment it stands along.
    //! \param[in] offset where along it, from 0 at segment.getFrom() to 1 at
    //! segment.getTo().
    //! \return the new building.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Path& path, Segment& segment, float offset);

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
    Zone& addZone(ZoneType const& type, CellRegion const& footprint);

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
                    Name const& searchTarget);

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
    //! city now, and unpainting a zone is not the same as bulldozing it.
    //! \param[in] zone the zone to unpaint.
    // -------------------------------------------------------------------------
    void removeZone(Zone& zone);

    // -------------------------------------------------------------------------
    //! \brief Demolish a segment, together with the buildings standing along
    //! it.
    //!
    //! The agents driving on it, or waiting at either end for it, are taken
    //! away rather than left addressing a road that is gone. What they carried
    //! goes back to the building that sent them out, or to another that will
    //! take it, so demolishing a road does not quietly destroy resources.
    //!
    //! \param[in] path the network the segment belongs to.
    //! \param[in] segment the segment to demolish.
    // -------------------------------------------------------------------------
    void removeSegment(Path& path, Segment& segment);

    // -------------------------------------------------------------------------
    //! \brief Demolish a crossroads, the segments touching it and the buildings
    //! standing on any of them.
    //!
    //! The agents concerned are recycled the same way as in removeSegment().
    //!
    //! \param[in] path the network the crossroads belongs to.
    //! \param[in] node the crossroads to demolish.
    // -------------------------------------------------------------------------
    void removeNode(Path& path, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Cut a segment in two and give back the junction, which is a
    //! crossroads an agent may stop at and a building may stand on.
    //!
    //! Path::splitSegment() only rewires the graph. Here the city also puts things
    //! back where they belong: the buildings that stood along the segment move
    //! onto the half now running under them, and the agents driving on it let
    //! go and route again from where they are, so none of them is left
    //! addressing a segment that stops short of it.
    //!
    //! This is what putting a building on a street does, which is why a street
    //! gains a crossroads every time a house is built on it.
    //!
    //! \param[in] path the network the segment belongs to.
    //! \param[in] segment the segment to cut.
    //! \param[in] offset where to cut, from 0 at segment.getFrom() to 1 at
    //! segment.getTo().
    //! \return the junction, or the end of the segment when the offset falls on
    //! one, in which case nothing was cut.
    // -------------------------------------------------------------------------
    Node& splitSegment(Path& path, Segment& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Move the city in the world, taking its roads, its buildings and
    //! its agents along. The cells it owns do not move: the rectangle is worked
    //! out again from the new position.
    //! \param[in] direction how far to move it, in world units.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief Which cell of the city a place falls in.
    //! \param[in] position the place, in world coordinates.
    //! \return the cell.
    //! \note Clamped to the cells of the city: a place outside them gives the
    //! nearest cell inside, not the cell it really falls in. Use
    //! Simulation::worldToCell() for the unclamped answer.
    // -------------------------------------------------------------------------
    Cell worldToCell(Vector3f const& position) const;

    // -------------------------------------------------------------------------
    //! \brief Where a cell sits in the world.
    //! \param[in] cell the cell.
    //! \return the world position of its top-left corner.
    // -------------------------------------------------------------------------
    Vector3f cellToWorld(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \brief \return the name of the city, unique in the World.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string const& getName() const
    {
        return m_name;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the world position of the top-left corner of its cells.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getPosition() const
    {
        return m_position;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the cells of the grid the city owns, worked out from its
    //! position and its size. This bounds every rule run on its behalf.
    //!
    //! Every command of a rule reaching into a layer asks for this, and a rule
    //! of a layer runs on every cell of it, so it is read hundreds of thousands
    //! of times per tick on a large city. It is therefore worked out once, when
    //! the city is founded or moved, and only read here.
    // -------------------------------------------------------------------------
    [[nodiscard]] CellRegion const& getRegion() const
    {
        return m_region;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the game calendar, shared by the whole world. This is
    //! what the opening hours of a building are read against.
    // -------------------------------------------------------------------------
    [[nodiscard]] SimulationClock const& getClock() const;

    // -------------------------------------------------------------------------
    //! \brief \return the runtime settings, held by the World and shared with
    //! every other city. Change them through Simulation::setConfig().
    // -------------------------------------------------------------------------
    [[nodiscard]] Config const& getConfig() const;

    // -------------------------------------------------------------------------
    //! \brief \return the side of a grid cell, in world units. Comes from the
    //! World, so every city of a world shares the same grid.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCellSize() const;

    // -------------------------------------------------------------------------
    //! \brief \return the resources belonging to the city as a whole rather
    //! than to a building: money, oil, electricity. This is what a \c global
    //! rule reads and writes.
    //!
    //! \note Writable, and the only accessor of City that is: a \c global rule
    //! is expected to spend and to earn.
    // -------------------------------------------------------------------------
    [[nodiscard]] Resources& getGlobals()
    {
        return m_globals;
    }

    //! \copydoc getGlobals()
    [[nodiscard]] Resources const& getGlobals() const
    {
        return m_globals;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the layers of the World, by name. Shared with the other
    //! cities.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layers const& getLayers() const;

    // -------------------------------------------------------------------------
    //! \brief \return the networks of the city, by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] Paths const& getPaths() const
    {
        return m_paths;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the buildings of the city, in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Units const& getUnits() const
    {
        return m_units;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the agents on the road, in creation order. The list turns
    //! over quickly: an agent lives from the building that sent it out to the
    //! one that takes its load.
    // -------------------------------------------------------------------------
    [[nodiscard]] Agents const& getAgents() const
    {
        return m_agents;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the zones painted on the city, in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Zones const& getZones() const
    {
        return m_zones;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the router the agents ask for an itinerary. One per city,
    //! so the memory it needs is allocated once instead of on every search.
    //!
    //! \note Const, though the router it hands out is not: a router is a
    //! service the city hosts, and using it does not change the city.
    // -------------------------------------------------------------------------
    [[nodiscard]] IRouter& getRouter() const
    {
        return *m_router;
    }

    // -------------------------------------------------------------------------
    //! \brief Install the router the agents use. It has to be set before an
    //! agent needs to travel. See installDijkstraRouter().
    //! \param[in] router the router, whose ownership is taken.
    // -------------------------------------------------------------------------
    void setRouter(std::unique_ptr<IRouter> router);

    // -------------------------------------------------------------------------
    //! \brief Empty the city: agents, buildings, zones, networks and globals.
    //!
    //! The layers of the World stay, being shared, and so does the ruleset:
    //! what the player drew goes away, what the script declared remains, so the
    //! city can be rebuilt without loading anything again.
    // -------------------------------------------------------------------------
    void clear();

    // -------------------------------------------------------------------------
    //! \brief Sweep away the crossroads left with no segment at all, which is
    //! what demolishing a street leaves behind. The agents standing on such a
    //! crossroads let go of it first.
    //! \param[in] path the network to sweep.
    // -------------------------------------------------------------------------
    void removeIsolatedNodes(Path& path) const;

    // -------------------------------------------------------------------------
    //! \brief Take away the agents left with no road under them, which is what
    //! happens to every one of them when the last street of the city is
    //! demolished. Their load is handed back the same way as in removeSegment().
    // -------------------------------------------------------------------------
    void removeStuckAgents();

private:

    // -------------------------------------------------------------------------
    //! \brief Work m_region out from the position of the city and the size of a
    //! grid cell. Called when the city is founded and whenever it moves.
    //!
    //! \note The size of a cell is settled when the World is built and is not
    //! meant to change afterwards. Changing it under a city that already exists
    //! would leave its cells behind.
    // -------------------------------------------------------------------------
    void updateRegion();

private:

    //! \brief Name of the city, unique in the World: "Paris", "Seattle", "NYC".
    std::string m_name;
    //! \brief The world holding the grid and the layers. Not owned.
    World& m_world;
    //! \brief World position of the top-left corner of the owned cells.
    Vector3f m_position;
    //! \brief How many owned cells along U.
    uint32_t m_gridSizeU;
    //! \brief How many owned cells along V.
    uint32_t m_gridSizeV;
    //! \brief The owned cells, kept in step with the three fields above by
    //! updateRegion().
    CellRegion m_region;
    //! \brief Identifier the next agent will be given. Never reused, so a stale
    //! identifier names nothing rather than something else.
    uint32_t m_nextAgentId = 0u;
    //! \brief Identifier the next building will be given.
    uint32_t m_nextUnitId = 0u;
    //! \brief Identifier the next zone will be given.
    uint32_t m_nextZoneId = 0u;
    //! \brief Resources belonging to the city as a whole: money, oil,
    //! electricity.
    Resources m_globals;
    //! \brief The networks, owned: roads, power lines, water pipes.
    Paths m_paths;
    //! \brief The buildings, owned: houses, factories, shops.
    Units m_units;
    //! \brief The agents on the road, owned: cars, citizens, trucks.
    Agents m_agents;
    //! \brief The zones painted by the player, owned.
    Zones m_zones;
    //! \brief The router, one per city so its scratch memory is allocated once
    //! rather than on every search.
    std::unique_ptr<IRouter> m_router;
    //! \brief The default callbacks, which do nothing.
    City::Listener m_defaultListener;
    //! \brief The registered callbacks. Not owned.
    City::Listener* m_listener = &m_defaultListener;
};

} // namespace ogb

#endif
