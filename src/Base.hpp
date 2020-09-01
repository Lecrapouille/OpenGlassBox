#ifndef BASE_HPP
#  define BASE_HPP

#  include "Resource.hpp"
#  include <vector>

struct ResourceType
{
    std::string              m_id;
    uint32_t                 m_capacity;
    uint32_t                 m_amount;
};

struct Unit;
struct UnitType
{
    std::string              m_id;
    uint32_t                 m_color;
    Node                     m_node;
    ResourceBin              m_resources;
    std::vector<RuleUnit>    m_rules;
    std::vector<std::string> m_targets;
};

struct SegmentType;
struct Path;
struct NodeType
{
    uint32_t                 m_id;
    vector3f                 m_position;
    Path*                    m_path;
    std::vector<SegmentType*> m_segments;
    std::vector<Unit*>        m_units;
};

struct SegmentType
{
    std::string              m_id;
    uint32_t                 m_color;
    NodeType*                m_point1;
    NodeType*                m_point2;
    PathType*                m_path;
};

struct PathType
{
    std::string              m_id;
    std::vector<NodeType>    m_nodes;
    std::vector<SegmentType> m_nodes;
};

struct AgentType
{
    std::string m_id;
    uint32_t    m_color;
    float       m_speed;
};

struct MapType
{
    std::string          m_id;
    uint32_t             m_color;
    uint32_t             m_capacity;
    std::vector<RuleMap> m_rules;
};

struct RuleContext
{
    Box box;
    Unit unit;
    ResourceBin localResources;
    ResourceBin globalResources;
    uint32_t mapPositionX;
    uint32_t mapPositionY;
    uint32_t mapPositionRadius;
};

#endif
