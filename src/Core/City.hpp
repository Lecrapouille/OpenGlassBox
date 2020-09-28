#ifndef CITY_HPP
#  define CITY_HPP

#  include "Core/Unit.hpp"
#  include "Core/Agent.hpp"
#  include "Core/Map.hpp"
#  include "Core/Path.hpp"

class Path;
class Node;

//==============================================================================
//! \brief Class holding everything that makes up a city.
//! Manage maps, global resources, Paths, Agents. Run Unit rule scripts.
//==============================================================================
class City
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create a empty city of a grid of gridSizeU x gridSizeV cells.
    // -------------------------------------------------------------------------
    City(std::string const& name, uint32_t gridSizeU, uint32_t gridSizeV);

    // -------------------------------------------------------------------------
    //! \brief Move agents, execute rule scripts of maps, execute rule scripts
    //! of Units.
    // -------------------------------------------------------------------------
    void update();

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Map.
    //! \note Remove the map of the same name if the identifier is already known.
    // -------------------------------------------------------------------------
    Map& addMap(MapType& type);

    // -------------------------------------------------------------------------
    //! \brief Get the reference of the Map or throw an exception if the Map
    //! does not exist.
    // -------------------------------------------------------------------------
    Map& getMap(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Path. Remove the map of the same name if
    //! the identifier is already known.
    // -------------------------------------------------------------------------
    Path& addPath(PathType& type);

    // -------------------------------------------------------------------------
    //! \brief Get the reference of the Path or throw an exception if the Path
    //! does not exist.
    // -------------------------------------------------------------------------
    Path& getPath(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Unit.
    // -------------------------------------------------------------------------
    Unit& addUnit(UnitType& type, Node& node);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Units& getUnits() { return m_units; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Paths& getPaths() { return m_paths; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Agent& addAgent(AgentType& type, Unit& owner, Resources const& resources,
                    std::string const& searchTarget);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Agents& getAgents() { return m_agents; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeAgent(Agent& agent);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier
    // -------------------------------------------------------------------------
    std::string const& name() const { return m_name; }

    // -------------------------------------------------------------------------
    //! \brief Change the position of the City in the world.
    //! This also change the position of Path, Unit, Agent ... hold by the City.
    // -------------------------------------------------------------------------
    // void setPosition(float x, float y, float, z); // TODO

    // -------------------------------------------------------------------------
    //! \brief Get the Map indice U and V from a world position.
    // -------------------------------------------------------------------------
    void world2mapPosition(Vector3f worldPos, uint32_t& u, uint32_t& v);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t gridSizeU() const { return m_gridSizeU; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t gridSizeV() const { return m_gridSizeV; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resources& globalResources() { return m_globals; }

private:

    //! \brief Name of the City, ie. Paris, Seattle, NYC ...
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
};

using Cities = std::map<std::string, std::unique_ptr<City>>;

#endif
