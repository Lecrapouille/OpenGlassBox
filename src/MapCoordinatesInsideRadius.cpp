//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/MapCoordinatesInsideRadius.hpp"
#include <random>

//------------------------------------------------------------------------------
// Random numbers
//------------------------------------------------------------------------------

// Will be used to obtain a seed for the random number engine
static std::random_device rd;
// Standard mersenne_twister_engine seeded with rd()
static std::mt19937 generator(rd());

//------------------------------------------------------------------------------
int32_t MapCoordinatesInsideRadius::compress(int32_t u, int32_t v)
{
    return ((u + MapCoordinatesInsideRadius::MAX_RADIUS) << 16) |
            (v + MapCoordinatesInsideRadius::MAX_RADIUS);
}

//------------------------------------------------------------------------------
void MapCoordinatesInsideRadius::uncompress(int32_t val, int32_t& u, int32_t& v)
{
    u = ((val >> 16) & 0xFFFF) - MapCoordinatesInsideRadius::MAX_RADIUS;
    v = (val & 0xFFFF) - MapCoordinatesInsideRadius::MAX_RADIUS;
}

//------------------------------------------------------------------------------
void MapCoordinatesInsideRadius::init(uint32_t radius,
                                      uint32_t centerU, uint32_t centerV,
                                      uint32_t minU, uint32_t maxU,
                                      uint32_t minV, uint32_t maxV,
                                      bool random)
{
    m_centerU = centerU;
    m_centerV = centerV;
    m_offset = 0u;
    m_minU = minU;
    m_maxU = maxU;
    m_minV = minV;
    m_maxV = maxV;

    // Create relative coordinates depending on the radius if it has not been
    // previously been in cache.
    RelativeCoordinates& relativeCoord = relativeCoordinates(radius);
    m_relativeCoord = &relativeCoord;
    if (relativeCoord.size() == 0u)
    {
        createRelativeCoordinates(int32_t(radius), relativeCoord);
    }

    if (random)
    {
        std::uniform_int_distribution<uint32_t> randomIndex(0u, uint32_t(relativeCoord.size() - 1u));
        m_startingIndex = randomIndex(generator);
    }
    else
    {
        m_startingIndex = 0u;
    }
}

//------------------------------------------------------------------------------
void MapCoordinatesInsideRadius::createRelativeCoordinates(int32_t radius, RelativeCoordinates &res)
{
    res.clear();
    for (int32_t u = -radius; u <= radius; ++u)
    {
        for (int32_t v = -radius; v <= radius; ++v)
        {
            if (std::abs(u) + std::abs(v) <= radius)
            {
                res.push_back(compress(u, v));
            }
        }
    }
}

//------------------------------------------------------------------------------
bool MapCoordinatesInsideRadius::next(uint32_t& u, uint32_t& v)
{
    RelativeCoordinates& coord = *m_relativeCoord;
    size_t const size = coord.size();
    while (m_offset < size)
    {
        int32_t iu; int32_t iv;
        uncompress(coord[(m_startingIndex + m_offset++) % size], iu, iv);
        u = uint32_t(iu) + m_centerU;
        v = uint32_t(iv) + m_centerV;

        if ((u >= m_minU) && (u < m_maxU) && (v >= m_minV) && (v < m_maxV))
            return true;
    }

    u = v = 0u;
    return false;
}
