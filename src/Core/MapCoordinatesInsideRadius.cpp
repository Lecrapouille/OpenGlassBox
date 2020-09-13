#include "Core/MapCoordinatesInsideRadius.hpp"
#include <random>

//------------------------------------------------------------------------------
// Random numbers
//------------------------------------------------------------------------------

// Will be used to obtain a seed for the random number engine
std::random_device rd;
// Standard mersenne_twister_engine seeded with rd()
std::mt19937 generator(rd());

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

    std::vector<uint32_t>& values = cachedCoordinates(radius);
    m_values = &values;
    if (values.size() == 0u)
    {
        createCoordinates(radius, values);
    }

    if (random)
    {
        std::uniform_int_distribution<uint32_t> randomIndex(0u, uint32_t(values.size()));
        m_startingIndex = randomIndex(generator);
    }
    else
    {
        m_startingIndex = 0u;
    }
}

void MapCoordinatesInsideRadius::createCoordinates(uint32_t radius, std::vector<uint32_t> &res)
{
    res.clear();
    //res.reserve(radius * radius);

    for (uint32_t u = -radius; u <= radius; ++u)
    {
        for (uint32_t v = -radius; v <= radius; ++v)
        {
            if (u + v <= radius)
            {
                res.push_back(((u + MAX_RADIUS) << 16) | (v + MAX_RADIUS));
            }
        }
    }
}

bool MapCoordinatesInsideRadius::next(uint32_t& u, uint32_t& v)
{
    size_t const size = m_values->size();
    while (m_offset < size)
    {
        uint32_t val = (*m_values)[(m_startingIndex + m_offset++) % size];

        u = ((val >> 16) & 0xFFFF) - MAX_RADIUS;
        v = (val & 0xFFFF) - MAX_RADIUS;

        u += m_centerU;
        v += m_centerV;

        if ((u >= m_minU) && (u < m_maxU) && (v >= m_minV) && (v < m_maxV))
            return true;
    }

    u = 0u;
    v = 0u;
    return false;
}
