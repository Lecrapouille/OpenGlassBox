#ifndef MAP_HPP
#  define MAP_HPP

#  include "Vector.hpp"
#  include "Config.hpp"
#  include <string>
#  include <vector>
#  include <limits>

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
    //! where the map capacity is infinite.
    // -------------------------------------------------------------------------
    Map(std::string const& id, uint32_t sizeX, uint32_t sizeY,
        uint32_t capacity = Map::MAX_CAPACITY);

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

    uint32_t getCapacity() const { return m_capacity; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    //uint32_t getCapacity(uint32_t const u, uint32_t const v);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    //uint32_t getCapacity(uint32_t const u, uint32_t const v, uint32_t const radius);

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
    std::string const& id() const { return m_id; }

    uint32_t gridSizeX() const { return m_sizeU; }
    uint32_t gridSizeY() const { return m_sizeV; }

public:

    //static const float GRID_SIZE;
    static const uint32_t MAX_CAPACITY;

private:

    std::string           m_id;
    uint32_t              m_sizeU;
    uint32_t              m_sizeV;
    uint32_t              m_ticks = 0u;
    uint32_t              m_capacity;
    std::vector<uint32_t> m_resources;
};

#endif
