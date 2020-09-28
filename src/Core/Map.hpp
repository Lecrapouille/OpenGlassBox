#ifndef MAP_HPP
#  define MAP_HPP

#  include "Core/Unique.hpp"
#  include "Core/Config.hpp"
#  include "Core/Rule.hpp"
#  include "Core/MapCoordinatesInsideRadius.hpp"
#  include "Core/Vector.hpp"

//==============================================================================
//! \brief Maps are simple uniform size grids. A Map represents a single type of
//! resource in the environment (coal, oil, forest but also air pollution, land
//! value, desirability ...). Each cell of a Map is a Resource. Units interact
//! with maps through their footprint. Resources are limited.
//==============================================================================
class Map
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
    uint32_t getCapacity() const { return m_type.capacity; }

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
    //! \brief Getter: return the type of Map.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t const& color() const { return m_type.color; }

    uint32_t gridSizeU() const { return m_sizeU; }
    uint32_t gridSizeV() const { return m_sizeV; }

private:

    MapType          const& m_type;
    uint32_t                m_sizeU;
    uint32_t                m_sizeV;
    uint32_t                m_ticks = 0u;
    std::vector<RuleMap>    m_rules;
    std::vector<uint32_t>   m_resources;
    MapCoordinatesInsideRadius m_coordinates;
};

using Maps = std::map<std::string, std::unique_ptr<Map>>;

#endif
