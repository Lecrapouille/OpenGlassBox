#ifndef CITY_HPP
#  define CITY_HPP

#  include "Resources.hpp"
#  include "Vector.hpp"
#  include <map>
#  include <string>
#  include <vector>
#  include <memory>

class Map;
class Unit;
class Path;
class Node;
class Agent;
class Resources;

//==============================================================================
//! \brief Class holding everything that makes up a city.
//! Manage maps, global resources, Paths, Agents. Run Unit rule scripts.
//==============================================================================
class City
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create a empty city.
    // -------------------------------------------------------------------------
    City(std::string const& id, uint32_t gridSizeX, uint32_t gridSizeY);

    // -------------------------------------------------------------------------
    //! \brief Move agents, execute rule scripts of maps, execute rule scripts
    //! of Units.
    // -------------------------------------------------------------------------
    void update();

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Map. Remove the map of the same name if
    //! the identifier is already known.
    // -------------------------------------------------------------------------
    Map& addMap(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Map& getMap(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Path. Remove the map of the same name if
    //! the identifier is already known.
    // -------------------------------------------------------------------------
    Map& addPath(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Path& getPath(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Unit.
    // -------------------------------------------------------------------------
    Unit& addUnit(std::string const& id, Node const& node);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Agent& addAgent(uint32_t id, Node const& node, Unit& owner,
                    Resources const& resources, std::string const& searchTarget);

    uint32_t gridSizeX() const { return m_gridSizeX; }
    uint32_t gridSizeY() const { return m_gridSizeY; }

private:

    using Maps = std::map<std::string, std::shared_ptr<Map>>;
    using Paths = std::map<std::string, std::shared_ptr<Path>>;
    using Units = std::vector<std::shared_ptr<Unit>>;
    using Agents = std::vector<std::shared_ptr<Agent>>;

    std::string   m_id;
    Vector3f      m_position;
    uint32_t      m_gridSizeX;
    uint32_t      m_gridSizeY;
    uint32_t      m_nextUnitId = 0u;
    uint32_t      m_nextAgentId = 0u;
    Resources     m_globals;
    Maps          m_maps;
    Paths         m_paths;
    Agents        m_agents;
};

#endif
