//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Map.hpp"
#include "Core/City.hpp"
#include "Config.hpp"

// -----------------------------------------------------------------------------
template<typename T>
static inline T clamp(T const value, T const lower, T const upper)
{
    if (value < lower)
        return lower;

    if (value > upper)
        return upper;

    return value;
}

// -----------------------------------------------------------------------------
Map::Map(MapType const& type, City& city)
    : m_type(type),
      m_position(city.position()),
      m_gridSizeU(city.gridSizeU()),
      m_gridSizeV(city.gridSizeV()),
      m_resources(m_gridSizeU * m_gridSizeV, 0u)
{
    m_context.city = &city;
}

// -----------------------------------------------------------------------------
void Map::setResource(uint32_t const u, uint32_t const v, uint32_t amount)
{
    if (amount > m_type.capacity)
        amount = m_type.capacity;

    uint32_t& res = m_resources[v * m_gridSizeU + u];
    if (res != amount)
        res = amount;
}

// -----------------------------------------------------------------------------
uint32_t Map::getResource(uint32_t const u, uint32_t const v)
{
    return m_resources[v * m_gridSizeU + u];
}

// -----------------------------------------------------------------------------
uint32_t Map::getResource(uint32_t const u, uint32_t const v, uint32_t radius)
{
    uint32_t totalInsideRadius = 0u;
    uint32_t x = u; uint32_t y = v;
    m_coordinates.init(radius, x, y, 0u, m_gridSizeU, 0u, m_gridSizeV, false);

    while (m_coordinates.next(x, y))
        totalInsideRadius += getResource(x, y);

    return totalInsideRadius;
}

// -----------------------------------------------------------------------------
void Map::addResource(uint32_t const u, uint32_t const v, uint32_t toAdd)
{
    uint32_t amount = getResource(u, v);

    // Avoid integer overflow
    if (amount >= Resource::MAX_CAPACITY - toAdd)
        amount = Resource::MAX_CAPACITY;
    else
        amount += toAdd;

    setResource(u, v, amount);
}

// -----------------------------------------------------------------------------
void Map::addResource(uint32_t const u, uint32_t const v, uint32_t const radius,
                      uint32_t toAdd, bool distributed)
{
    uint32_t remainingToAdd = toAdd;
    uint32_t x = u; uint32_t y = v;

    m_coordinates.init(radius, x, y, 0, m_gridSizeU, 0, m_gridSizeV, distributed);
    while ((remainingToAdd > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toAdd = std::min(m_type.capacity - amount, remainingToAdd);
        if (toAdd > 0u)
        {
            amount += toAdd;
            if (distributed) { remainingToAdd -= toAdd; }
            setResource(x, y, amount);
        }
    }
}

// -----------------------------------------------------------------------------
void Map::removeResource(uint32_t const u, uint32_t const v, uint32_t toRemove)
{
    uint32_t amount = getResource(u, v);

    if (amount > toRemove)
        amount -= toRemove;
    else
        amount = 0u;

    setResource(u, v, amount);
}

// -----------------------------------------------------------------------------
void Map::removeResource(uint32_t const u, uint32_t const v, uint32_t radius,
                         uint32_t toRemove, bool distributed)
{
    uint32_t remainingToRemove = toRemove;
    uint32_t x = u; uint32_t y = v;

    m_coordinates.init(radius, x, y, 0u, m_gridSizeU, 0u, m_gridSizeV, distributed);
    while ((remainingToRemove > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toRemove = std::min(amount, remainingToRemove);
        if (toRemove > 0u)
        {
            amount -= toRemove;
            if (distributed) { remainingToRemove -= toRemove; }
            setResource(x, y, amount);
        }
    }
}

// -----------------------------------------------------------------------------
Vector3f Map::getWorldPosition(uint32_t const u, uint32_t const v)
{
    return Vector3f(float(clamp(u, 0u, m_gridSizeU)) * config::GRID_SIZE,
                    float(clamp(v, 0u, m_gridSizeV)) * config::GRID_SIZE,
                    0.0f);
}

// -----------------------------------------------------------------------------
void Map::translate(Vector3f const direction)
{
    m_position += direction;
}

// -----------------------------------------------------------------------------
void Map::executeRules()
{
    ++m_ticks;

    for (auto& rule: m_type.rules)
    {
        if (m_ticks % rule->rate() == 0u)
        {
            if (rule->isRandom())
            {
                m_randomCoordinates.init(m_gridSizeU, m_gridSizeV);
                uint32_t tilesAmount = rule->percent(m_gridSizeU * m_gridSizeV);
                while (tilesAmount--)
                {
                    if (m_randomCoordinates.next(m_context.u, m_context.v))
                        rule->execute(m_context);
                }
            }
            else
            {
                uint32_t u = m_gridSizeU;
                while (u--)
                {
                    m_context.u = u;
                    uint32_t v = m_gridSizeV;
                    while (v--)
                    {
                        m_context.v = v;
                        rule->execute(m_context); // FIXME return bool useless ?
                    }
                }
            }
        }
    }
}
