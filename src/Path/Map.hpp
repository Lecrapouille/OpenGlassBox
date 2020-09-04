#ifndef MAP_HPP
#define MAP_HPP

//==============================================================================
//! \brief Maps are simple uniform size grids. A Map represents a single type of
//! resource in the environment (coal, oil, forest but also air pollution, land
//! value, desirability ...). Each cell of a Map is a Resource. Units interact
//! with maps through their footprint. Resources are limited.
//==============================================================================
class Map
{
public:

    Map(uint32_t sizeX, uint32_t sizeY);

public:

    static const float GRID_SIZE;

private:

    std::string           m_id;
    uint32_t              m_sizeX;
    uint32_t              m_sizeY;
    uint32_t              m_capacity = 0u;
    std::vector<uint32_t> m_resources;
};

#endif
