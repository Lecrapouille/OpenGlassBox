#ifndef BOX_HPP
#  define BOX_HPP

#  include "Map.hpp"
#  include "Path.hpp"
#  include "Agent.hpp"
#  include <map>
#  include <string>
#  include <vector>
#  include <memory>

class Box
{
public:

    Box(std::string const& id, uint32_t gridSizeX, uint32_t gridSizeY);
    void update();
    Map& addMap(std::string const& id);
    Map& getMap(std::string const& id);
    Map& addPath(std::string const& id);
    Path& getPath(std::string const& id);
    Unit& addUnit(std::string const& id, Node const& node);
    Agent& addAgent(uint32_t id, Node const& node, Unit& owner,
                    ResourceBin const& resources, string const& searchTarget);

private:

    using Maps = std::map<std::string, std::shared_ptr<Map>>;
    using Pats = std::map<std::string, std::shared_ptr<Path>>;
    using Units = std::vector<std::shared_ptr<Unit>>;
    using Agents = std::vector<std::shared_ptr<Agent>>;

    std::string   m_id;
    Simulation   *m_simulation;
    Vector3f      m_position;
    uint32_t      m_gridSizeX;
    uint32_t      m_gridSizeY;
    uint32_t      m_nextUnitId = 0u;
    uint32_t      m_nextAgentId = 0u;
    ResourceBin   m_globals;
    Maps          m_maps;
    Paths         m_paths;
    Agents        m_agents;
};

#endif
