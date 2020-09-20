#ifndef MAP_HPP
#  define MAP_HPP

#  include "Core/Vector.hpp"
#  include "Core/Unique.hpp"
#  include "Core/Config.hpp"
#  include "Core/Rule.hpp"
#  include "Core/MapCoordinatesInsideRadius.hpp"
#  include <string>
#  include <vector>
#  include <limits>
#  include <map>

//==========================================================================
//! \brief Type of Map (water, grass ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - map Water color 0x0000FF capacity 100 rules [ ]
//!  - map Grass color 0x00FF00 capacity 10 rules [ CreateGrass ]
//==========================================================================
class MapType
{
public:

    MapType() = default;
    MapType(MapType const&) = default;

    MapType(std::string const& name)
        : m_id(name), m_color(0xFFFFFF), m_capacity(MapType::MAX_CAPACITY)
    {}

    MapType(std::string const& name, uint32_t color, uint32_t capacity,
         std::initializer_list<RuleMap> list = {})
        : m_id(name), m_color(color), m_capacity(capacity), m_rules(list)
    {}

    std::string          m_id;
    uint32_t             m_color;
    uint32_t             m_capacity;
    std::vector<RuleMap> m_rules;

public:

    static const uint32_t MAX_CAPACITY;
};

//==============================================================================
//! \brief Maps are simple uniform size grids. A Map represents a single type of
//! resource in the environment (coal, oil, forest but also air pollution, land
//! value, desirability ...). Each cell of a Map is a Resource. Units interact
//! with maps through their footprint. Resources are limited.
//==============================================================================
class Map: private MapType
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create a sizeU x sizeV grid where each cell has no resource but
    //! where the map capacity and rules are defined by the type of map.
    // -------------------------------------------------------------------------
    Map(MapType const& type, uint32_t sizeU, uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief Change the amount of resource at the cell index U,V.
    // -------------------------------------------------------------------------
    void setResource(uint32_t const u, uint32_t const v, uint32_t val);

    // -------------------------------------------------------------------------
    //! \brief Get the amount of resource at the cell index U,V.
    // -------------------------------------------------------------------------
    uint32_t getResource(uint32_t const u, uint32_t const v);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getResource(uint32_t const u, uint32_t const v, uint32_t radius);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getCapacity() const { return m_capacity; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void addResource(uint32_t const u, uint32_t const v, uint32_t toAdd);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void addResource(uint32_t const u, uint32_t const v, uint32_t const radius, uint32_t const toAdd);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResource(uint32_t const u, uint32_t const v, uint32_t toRemove);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResource(uint32_t const u, uint32_t const v, uint32_t radius, uint32_t toRemove);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Vector3f getWorldPosition(uint32_t const u, uint32_t const v);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t const& color() const { return m_color; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t const& capacity() const { return m_capacity; }

    uint32_t gridSizeX() const { return m_sizeU; }
    uint32_t gridSizeY() const { return m_sizeV; }

private:

    uint32_t                m_sizeU;
    uint32_t                m_sizeV;
    uint32_t                m_ticks = 0u;
    std::vector<RuleMap>    m_rules;
    std::vector<uint32_t>   m_resources;
    MapCoordinatesInsideRadius m_coordinates;
};

using Maps = std::map<std::string, std::unique_ptr<Map>>;

#endif
