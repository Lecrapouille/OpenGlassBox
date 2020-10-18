//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_CITY_HPP
#  define OPEN_GLASSBOX_CITY_HPP

#  include "Core/Unit.hpp"
#  include "Core/Agent.hpp"
#  include "Core/Map.hpp"
#  include "Core/Path.hpp"
#  include "Core/Dijkstra.hpp"

class Path;
class Node;

//==============================================================================
//! \brief Class holding everything that makes up a city.
//! Manage maps, global resources, Paths, Agents. Run Unit and Map rule scripts.
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
    };

public:

    // -------------------------------------------------------------------------
    //! \brief Create a empty city of a grid of gridSizeU x gridSizeV cells at
    //! the given position in the world coordinate.
    // -------------------------------------------------------------------------
    City(std::string const& name, Vector3f position, uint32_t gridSizeU, uint32_t gridSizeV);

    // -------------------------------------------------------------------------
    //! \brief Create a empty city of a grid of gridSizeU x gridSizeV cells at
    //! position 0 in the world coordinate.
    // -------------------------------------------------------------------------
    City(std::string const& name, uint32_t gridSizeU, uint32_t gridSizeV);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setListener(City::Listener& listener);

    // -------------------------------------------------------------------------
    //! \brief Create a empty city of a grid of 32 x 32 cells at position 0 in
    //! the world coordinate.
    // -------------------------------------------------------------------------
    City(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Move agents, execute rule scripts of maps, execute rule scripts
    //! of Units.
    // -------------------------------------------------------------------------
    void update();

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Map. Destroy the Map of the same name if
    //! the identifier is already known.
    //! \param[in] type: the type of Map parsed from the simulation script.
    //! \return the newly created Map instance.
    // -------------------------------------------------------------------------
    Map& addMap(MapType const& type);

    // -------------------------------------------------------------------------
    //! \brief Get the reference of the Map or throw an exception if the Map
    //! does not exist.
    //! \param[in] name: the unique identifier of the Map.
    //! \throw std::exception if the Map is not found.
    //! \return the instance of the Map.
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
    //! \brief Create a new node splitting a Way into two ways and attach an
    //! Unit to this new node.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType const& type, Path& path, Way& way, float offset);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Agent& addAgent(AgentType const& type, Unit& owner, Resources const& resources,
                    std::string const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief Translate the position of the City inside the world coordinate.
    //! This also change the position of Path, Unit, Agent ... hold by the City.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

    // -------------------------------------------------------------------------
    //! \brief Get the Map indice U and V from a given position inside the world
    //! coordinate.
    //! \param[in] worldPos: the world position.
    //! \param[out] u: the grid indice along the U-axis.
    //! \param[out] v: the grid indice along the V-axis.
    // -------------------------------------------------------------------------
    void world2mapPosition(Vector3f worldPos, uint32_t& u, uint32_t& v);

    // -------------------------------------------------------------------------
    //! \brief Return the name (unique identifier).
    // -------------------------------------------------------------------------
    std::string const& name() const { return m_name; }

    // -------------------------------------------------------------------------
    //! \brief Return the position inside the World coordinate of the city (top-left corner).
    // -------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief Return the number of graduations along the U-axis
    // -------------------------------------------------------------------------
    uint32_t gridSizeU() const { return m_gridSizeU; }

    // -------------------------------------------------------------------------
    //! \brief Return the number of graduations along the V-axis
    // -------------------------------------------------------------------------
    uint32_t gridSizeV() const { return m_gridSizeV; }

    // -------------------------------------------------------------------------
    //! \brief Return global resources.
    // -------------------------------------------------------------------------
    Resources& globals() { return m_globals; }

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Maps.
    // -------------------------------------------------------------------------
    Maps& maps() { return m_maps; }

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

    // -------------------------------------------------------------------------
    //! \brief Return the collection of Maps.
    // -------------------------------------------------------------------------
    Maps const& maps() const { return m_maps; }

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

private:

    //! \brief Name of the City, ie. "Paris", "Seattle", "NYC" ...
    std::string   m_name;
    //! \brief Position of the top-left corner.
    Vector3f      m_position;
    //! \brief The size of the grid along the U-axis.
    uint32_t      m_gridSizeU;
    //! \brief The size of the grid along the V-axis.
    uint32_t      m_gridSizeV;
    //! \brief Counter of Unit to create unique id.
    uint32_t      m_nextUnitId = 0u;
    //! \brief Counter of Agent to create unique id.
    uint32_t      m_nextAgentId = 0u;
    //! \brief Globals resources (money, oil, electricity ...)
    Resources     m_globals;
    //! \brief Collection of resources in the environement
    Maps          m_maps;
    //! \brief Collection of graphs (roads, power lines, water pipes ...)
    Paths         m_paths;
    //! \brief Collection of building (house, factory ...)
    Units         m_units;
    //! \brief Collection of resource carrier (cars, citizens ...)
    Agents        m_agents;
    //! \brief
    Dijkstra      m_dijkstra;
    //!
    City::Listener *m_listener;
};

//==============================================================================
//! \brief Collection of City.
//==============================================================================
using Cities = std::map<std::string, std::unique_ptr<City>>;

#endif
