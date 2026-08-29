//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file City.hpp
//! \brief One city: roads, buildings, agents, and zones.

#ifndef OPEN_GLASSBOX_CITY_HPP
#define OPEN_GLASSBOX_CITY_HPP

#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/Layer.hpp"
#include "OpenGlassBox/CellRegion.hpp"
#include "OpenGlassBox/Router.hpp"
#include "OpenGlassBox/Building.hpp"

namespace ogb
{

class Path;
class Node;
class World;
class SimulationClock;

//==============================================================================
//! \brief One city with roads, buildings, agents, and zones.
//!
//! Each tick moves agents on roads, runs building and zone rules, and updates
//! street traffic averages. The city owns these objects and returns references
//! to them.
//!
//! The city does not own environment layers. The World owns layers and shares
//! them between cities. Pollution does not stop at a city border. The city
//! owns a rectangle of cells on the grid. That rectangle bounds every rule run
//! for this city. Two cities can read the same layer without changing each
//! other.
//!
//! This class is not thread safe. Do not add or remove objects during a tick.
//! The demo queues player edits and applies them between ticks.
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
//! city.addBuilding(rules.getBuildingType("Home"), a);
//! city.addBuilding(rules.getBuildingType("Work"), b);
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
    //! \brief Callbacks that tell a renderer what changed in the city.
    //!
    //! A renderer can use these callbacks instead of scanning the whole city
    //! each frame. Each method does nothing by default. Override only what you
    //! need. References passed to a callback are valid only during the call. A
    //! removal callback runs while the object still exists. The object is gone
    //! when the callback returns.
    //==========================================================================
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //! \brief The World made a layer available to this city.
        virtual void onLayerAdded(Layer& /*layer*/) {}

        //! \brief A layer will be removed.
        virtual void onLayerRemoved(Layer& /*layer*/) {}

        //! \brief A network was created. It has no crossroads or segments yet.
        virtual void onPathAdded(Path& /*path*/) {}

        //! \brief A network will be removed, with all its contents.
        virtual void onPathRemoved(Path& /*path*/) {}

        //! \brief A building was added by the player or by a zone.
        virtual void onBuildingAdded(Building& /*building*/) {}

        //! \brief A building will be removed.
        virtual void onBuildingRemoved(Building& /*building*/) {}

        //! \brief A building sent out an agent.
        virtual void onAgentAdded(Agent& /*agent*/) {}

        //! \brief An agent will be removed. It arrived or lost its road.
        virtual void onAgentRemoved(Agent& /*agent*/) {}

        //! \brief A zone was painted.
        virtual void onZoneAdded(Zone& /*zone*/) {}

        //! \brief A zone will be removed. Its buildings stay.
        virtual void onZoneRemoved(Zone& /*zone*/) {}
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Create a city over a rectangle of the world grid.
    //!
    //! Prefer Simulation::addCity(). It also registers the city in the world.
    //! A City built directly is unknown to the World. Layers it requests will
    //! not run.
    //!
    //! \param[in] name Unique city name, such as "Paris".
    //! \param[in] position World position of the top-left corner of its cells.
    //! \param[in] sizeU Number of cells along U.
    //! \param[in] sizeV Number of cells along V.
    //! \param[in] world The world this city belongs to. Must outlive the City.
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
    //! \brief Register callbacks. A second call replaces the first listener.
    //! \param[in] listener Kept by address. Must outlive the City.
    // -------------------------------------------------------------------------
    void setListener(City::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief Run one tick: move agents, run building and zone rules, update
    //! traffic averages.
    //!
    //! Layer rules do not run here. The World runs them once for all cities.
    //!
    //! \param[in] dt Tick duration in seconds of game time. Used to turn agent
    //! speed into distance.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief Run one tick using the duration from settings. The game loop
    //! calls this.
    // -------------------------------------------------------------------------
    void update();

    // -------------------------------------------------------------------------
    //! \brief Request a layer from the World. The World creates it when the
    //! first city needs it.
    //! \param[in] type Layer type from the ruleset.
    //! \return The layer. The World owns it and shares it with other cities.
    // -------------------------------------------------------------------------
    Layer& addLayer(LayerType const& type);

    // -------------------------------------------------------------------------
    //! \brief Find a layer by name.
    //! \param[in] name Layer name, such as "Water".
    //! \return The layer.
    //! \throw std::out_of_range if no city ever requested that layer.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layer& getLayer(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Create a network. An existing network with the same name is
    //! removed first, with its crossroads and segments.
    //! \param[in] type Network type from the ruleset.
    //! \return The new empty network.
    // -------------------------------------------------------------------------
    Path& addPath(PathType const& type);

    // -------------------------------------------------------------------------
    //! \brief Find a network by name.
    //! \param[in] name Network name, such as "Road".
    //! \return The network.
    //! \throw std::out_of_range if the city has no such network.
    // -------------------------------------------------------------------------
    [[nodiscard]] Path& getPath(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Place a building on a crossroads. The crossroads becomes its
    //! address.
    //! \param[in] type Building type from the ruleset.
    //! \param[in] node Crossroads where it stands. An agent at that crossroads
    //! has reached the building.
    //! \return The new building.
    // -------------------------------------------------------------------------
    Building& addBuilding(BuildingType const& type, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Place a building along a segment without splitting it. A street
    //! with forty houses stays one segment.
    //!
    //! An agent on the segment may deliver to this building. Routing targets
    //! the nearer segment end.
    //!
    //! \param[in] type Building type from the ruleset.
    //! \param[in] path Network that owns the segment.
    //! \param[in] segment Segment where the building stands.
    //! \param[in] offset Position along the segment, from 0 at segment.getFrom()
    //! to 1 at segment.getTo().
    //! \return The new building.
    // -------------------------------------------------------------------------
    Building& addBuilding(BuildingType const& type, Path& path, Segment& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Place a building with no road attachment.
    //!
    //! The building runs its rules, but no agent can reach it. Agents it sends
    //! out have no destination. Used in tests and when a zone has no road yet.
    //!
    //! \param[in] type Building type from the ruleset.
    //! \param[in] position World position.
    //! \return The new building.
    // -------------------------------------------------------------------------
    Building& addBuilding(BuildingType const& type, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Paint a zone over a rectangle of cells.
    //! \param[in] type Zone type from the ruleset.
    //! \param[in] footprint Cells the zone covers on the World grid.
    //! \return The new zone. It starts growing buildings on the next tick.
    // -------------------------------------------------------------------------
    Zone& addZone(ZoneType const& type, CellRegion const& footprint);

    // -------------------------------------------------------------------------
    //! \brief Send an agent out of a building with a load and a search target.
    //! Rules call this. Manual calls are rare.
    //!
    //! \param[in] type Agent type from the ruleset: speed, draw color.
    //! \param[in] owner Building the agent leaves. Its starting position.
    //! \param[in] resources Load the agent carries. Copied to the agent.
    //! \param[in] searchTarget Resource name a building will accept, such as
    //! "People".
    //! \return The new agent.
    // -------------------------------------------------------------------------
    Agent& addAgent(AgentType const& type,
                    Building& owner,
                    Resources const& resources,
                    Name const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief Remove a building from its crossroads or segment. The road stays.
    //!
    //! Agents from this building keep going. They search for a building type,
    //! not this building. They find another or are removed with nowhere to
    //! unload.
    //!
    //! \param[in] building Building to remove. Nothing may refer to it after this
    //! call.
    // -------------------------------------------------------------------------
    void removeBuilding(Building& building);

    // -------------------------------------------------------------------------
    //! \brief Remove a zone. Its buildings stay in the city. Removing a zone is
    //! not the same as demolishing its buildings.
    //! \param[in] zone Zone to remove.
    // -------------------------------------------------------------------------
    void removeZone(Zone& zone);

    // -------------------------------------------------------------------------
    //! \brief Remove a segment and buildings along it.
    //!
    //! Agents on the segment or waiting at its ends are removed. Their load
    //! goes back to the sender or to another building that accepts it. Removing
    //! a road does not destroy resources.
    //!
    //! \param[in] path Network that owns the segment.
    //! \param[in] segment Segment to remove.
    // -------------------------------------------------------------------------
    void removeSegment(Path& path, Segment& segment);

    // -------------------------------------------------------------------------
    //! \brief Remove a crossroads, its connected segments, and buildings on
    //! them.
    //!
    //! Agents are handled the same way as in removeSegment().
    //!
    //! \param[in] path Network that owns the crossroads.
    //! \param[in] node Crossroads to remove.
    // -------------------------------------------------------------------------
    void removeNode(Path& path, Node& node);

    // -------------------------------------------------------------------------
    //! \brief Move a crossroads, taking with it the roads that meet there and
    //! what stands on them.
    //!
    //! The roads keep their ends and become as long as they now look, which is
    //! what the router charges an agent for. Buildings read their position back
    //! from the road they stand on, unless a zone gave them a footprint of
    //! their own. Agents keep going where they were going, over roads whose
    //! cost changed, so their itineraries are computed again.
    //!
    //! \param[in] node Crossroads to move.
    //! \param[in] position Where it goes, in world units.
    // -------------------------------------------------------------------------
    void moveNode(Node& node, Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Split a segment and return the new junction crossroads.
    //!
    //! Path::splitSegment() only rewires the graph. This method also updates
    //! buildings and agents. Buildings along the segment move to the correct
    //! half. Agents on the segment reroute from their current position.
    //!
    //! Placing a building on a street uses this. Each house adds a crossroads
    //! to the street.
    //!
    //! \param[in] path Network that owns the segment.
    //! \param[in] segment Segment to split.
    //! \param[in] offset Split position, from 0 at segment.getFrom() to 1 at
    //! segment.getTo().
    //! \return The junction, or a segment end if offset is on an end. No split
    //! happens in that case.
    // -------------------------------------------------------------------------
    Node& splitSegment(Path& path, Segment& segment, float offset);

    // -------------------------------------------------------------------------
    //! \brief Move the city in the world. Roads, buildings, and agents move
    //! with it. Owned cells do not move. The cell rectangle is recomputed from
    //! the new position.
    //! \param[in] direction Move vector in world units.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief Return the city cell for a world position.
    //! \param[in] position Position in world coordinates.
    //! \return The cell.
    //! \note Result is clamped to city cells. A position outside returns the
    //! nearest inside cell. Use Simulation::worldToCell() for the unclamped
    //! cell.
    // -------------------------------------------------------------------------
    Cell worldToCell(Vector3f const& position) const;

    // -------------------------------------------------------------------------
    //! \brief Return the world position of a cell corner.
    //! \param[in] cell The cell.
    //! \return World position of its top-left corner.
    // -------------------------------------------------------------------------
    Vector3f cellToWorld(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \return City name, unique in the World.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string const& getName() const
    {
        return m_name;
    }

    // -------------------------------------------------------------------------
    //! \return World position of the top-left corner of its cells.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getPosition() const
    {
        return m_position;
    }

    // -------------------------------------------------------------------------
    //! \return Cells the city owns, from its position and size. This
    //! bounds every rule run for this city.
    //!
    //! Rule commands read this often. A layer rule runs on every cell in this
    //! region. The value is computed once when the city is created or moved.
    // -------------------------------------------------------------------------
    [[nodiscard]] CellRegion const& getRegion() const
    {
        return m_region;
    }

    // -------------------------------------------------------------------------
    //! \return Game clock shared by the world. Buildings use it for
    //! opening hours.
    // -------------------------------------------------------------------------
    [[nodiscard]] SimulationClock const& getClock() const;

    // -------------------------------------------------------------------------
    //! \return Runtime settings from the World, shared by all cities.
    //! Change them through Simulation::setConfig().
    // -------------------------------------------------------------------------
    [[nodiscard]] Config const& getConfig() const;

    // -------------------------------------------------------------------------
    //! \return Grid cell side length in world units. Comes from the
    //! World. All cities in a world share the same grid.
    // -------------------------------------------------------------------------
    [[nodiscard]] float getCellSize() const;

    // -------------------------------------------------------------------------
    //! \return City-wide resources: money, oil, electricity. Global
    //! rules read and write this.
    //!
    //! \note Writable. This is the only non-const City accessor. Global rules
    //! spend and earn resources.
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
    //! \return World layers by name, shared with other cities.
    // -------------------------------------------------------------------------
    [[nodiscard]] Layers const& getLayers() const;

    // -------------------------------------------------------------------------
    //! \return City networks by name.
    // -------------------------------------------------------------------------
    [[nodiscard]] Paths const& getPaths() const
    {
        return m_paths;
    }

    // -------------------------------------------------------------------------
    //! \return City buildings in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Buildings const& getBuildings() const
    {
        return m_buildings;
    }

    // -------------------------------------------------------------------------
    //! \return Agents on roads in creation order. The list changes
    //! often. An agent lives from send to delivery.
    // -------------------------------------------------------------------------
    [[nodiscard]] Agents const& getAgents() const
    {
        return m_agents;
    }

    // -------------------------------------------------------------------------
    //! \return Painted zones in creation order.
    // -------------------------------------------------------------------------
    [[nodiscard]] Zones const& getZones() const
    {
        return m_zones;
    }

    // -------------------------------------------------------------------------
    //! \return Router agents use for routes. One router per city saves
    //! memory on each search.
    //!
    //! \note Const accessor. The returned router is not const. Using it does
    //! not change the city.
    // -------------------------------------------------------------------------
    [[nodiscard]] IRouter& getRouter() const
    {
        return *m_router;
    }

    // -------------------------------------------------------------------------
    //! \brief Set the router agents use. Set it before agents need routes. See
    //! installDijkstraRouter().
    //! \param[in] router Router. The city takes ownership.
    // -------------------------------------------------------------------------
    void setRouter(std::unique_ptr<IRouter> router);

    // -------------------------------------------------------------------------
    //! \brief Remove all agents, buildings, zones, networks, and globals.
    //!
    //! World layers and the ruleset stay. Player content is removed. Script
    //! definitions stay. The city can be rebuilt without reloading.
    // -------------------------------------------------------------------------
    void clear();

    // -------------------------------------------------------------------------
    //! \brief Remove crossroads with no segments. Demolishing a street can
    //! leave these. Agents on such crossroads release them first.
    //! \param[in] path Network to clean.
    // -------------------------------------------------------------------------
    void removeIsolatedNodes(Path& path) const;

    // -------------------------------------------------------------------------
    //! \brief Remove agents with no road under them. This happens when the last
    //! street is removed. Load is returned the same way as in removeSegment().
    // -------------------------------------------------------------------------
    void removeStuckAgents();

private:

    // -------------------------------------------------------------------------
    //! \brief Recompute m_region from city position and cell size. Called at
    //! creation and on move.
    //!
    //! \note Cell size is fixed when the World is created. Changing it later
    //! would leave existing city cells behind.
    // -------------------------------------------------------------------------
    void updateRegion();

private:

    //! \brief City name, unique in the World.
    std::string m_name;
    //! \brief World that holds the grid and layers. Not owned.
    World& m_world;
    //! \brief World position of the top-left corner of owned cells.
    Vector3f m_position;
    //! \brief Number of owned cells along U.
    uint32_t m_gridSizeU;
    //! \brief Number of owned cells along V.
    uint32_t m_gridSizeV;
    //! \brief Owned cells. updateRegion() keeps this in sync.
    CellRegion m_region;
    //! \brief Next agent id. Never reused.
    uint32_t m_nextAgentId = 0u;
    //! \brief Next building id.
    uint32_t m_nextBuildingId = 0u;
    //! \brief Next zone id.
    uint32_t m_nextZoneId = 0u;
    //! \brief City-wide resources: money, oil, electricity.
    Resources m_globals;
    //! \brief Owned networks: roads, power lines, water pipes.
    Paths m_paths;
    //! \brief Owned buildings: houses, factories, shops.
    Buildings m_buildings;
    //! \brief Owned agents on roads: cars, citizens, trucks.
    Agents m_agents;
    //! \brief Owned zones painted by the player.
    Zones m_zones;
    //! \brief Router for this city. One per city saves search memory.
    std::unique_ptr<IRouter> m_router;
    //! \brief Default callbacks that do nothing.
    City::Listener m_defaultListener;
    //! \brief Registered callbacks. Not owned.
    City::Listener* m_listener = &m_defaultListener;
};

} // namespace ogb

#endif
