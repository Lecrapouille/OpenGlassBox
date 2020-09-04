#ifndef MAP_HPP
#  define MAP_HPP

#  include "Vector.hpp"
#  include <string>
#  include <vector>

//==============================================================================
//! \brief Maps are simple uniform size grids. A Map represents a single type of
//! resource in the environment (coal, oil, forest but also air pollution, land
//! value, desirability ...). Each cell of a Map is a Resource. Units interact
//! with maps through their footprint. Resources are limited.
//==============================================================================
class Map
{
public:

    Map(std::string const& id, uint32_t sizeX, uint32_t sizeY);
    void setValue(uint32_t const x, uint32_t const y, uint32_t val);
    uint32_t getValue(uint32_t const x, uint32_t const y);
    uint32_t getValue(uint32_t const x, uint32_t const y, uint32_t radius);
    uint32_t getCapacity(uint32_t const x, uint32_t const y);
    uint32_t getCapacity(uint32_t const x, uint32_t const y, uint32_t const radius);
    void add(uint32_t const x, uint32_t const y, uint32_t toAdd);
    void add(uint32_t const x, uint32_t const y, uint32_t const radius, uint32_t const toAdd);
    void remove(uint32_t const x, uint32_t const y, uint32_t toRemove);
    void remove(uint32_t const x, uint32_t const y, uint32_t radius, uint32_t toRemove);
    Vector3f getWorldPosition(uint32_t const x, uint32_t const y);
    void executeRules();

public:

    static const float GRID_SIZE;

private:

    std::string           m_id;
    uint32_t              m_sizeX;
    uint32_t              m_sizeY;
    uint32_t              m_ticks = 0u;
    uint32_t              m_capacity = 0u;
    std::vector<uint32_t> m_resources;
};

#endif
