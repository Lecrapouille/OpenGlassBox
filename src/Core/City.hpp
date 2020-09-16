#ifndef CITY_HPP
#  define CITY_HPP

#  include "Core/Resources.hpp"
#  include "Core/Vector.hpp"
#  include "Core/Unit.hpp"
#  include "Core/Agent.hpp"
#  include "Core/Map.hpp"
#  include <map>
#  include <string>
#  include <vector>

class Path;
class Node;
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

    City() = default;

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
    Path& addPath(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Path& getPath(std::string const& id);

    // -------------------------------------------------------------------------
    //! \brief Create and store a new Unit.
    // -------------------------------------------------------------------------
    Unit& addUnit(std::string const& id, Node& node);

    Units& getUnits() { return m_units; }
    Paths& getPaths() { return m_paths; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Agent& addAgent(Agent::Type const& type, Unit& owner,
                    Resources const& resources, std::string const& searchTarget);

    Agents& getAgents() { return m_agents; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeAgent(Agent& agent);

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier
    // -------------------------------------------------------------------------
    std::string const& id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Get the Map indice U and V from a world position.
    // -------------------------------------------------------------------------
    void world2mapPosition(Vector3f worldPos, uint32_t& u, uint32_t& v);

    uint32_t gridSizeX() const { return m_gridSizeX; }
    uint32_t gridSizeY() const { return m_gridSizeY; }
    Resources& globalResources() { return m_globals; }

private:

    std::string   m_id;
    Vector3f      m_position;
    uint32_t      m_gridSizeX;
    uint32_t      m_gridSizeY;
    uint32_t      m_nextUnitId = 0u;
    uint32_t      m_nextAgentId = 0u;
    Resources     m_globals;
    Maps          m_maps;
    Paths         m_paths;
    Units         m_units;
    Agents        m_agents;
};

using Cities = std::map<std::string, std::unique_ptr<City>>;

#endif
