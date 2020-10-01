//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Map.hpp"

template<typename T>
static inline T clamp(T const value, T const lower, T const upper)
{
    if (value < lower)
        return lower;

    if (value > upper)
        return upper;

    return value;
}

Map::Map(MapType const& type, uint32_t sizeU, uint32_t sizeV)
    : m_type(type),
      m_sizeU(sizeU), m_sizeV(sizeV),
      m_resources(sizeU * sizeV, 0u)
{}

void Map::setResource(uint32_t const u, uint32_t const v, uint32_t amount)
{
    if (amount > m_type.capacity)
        amount = m_type.capacity;

    uint32_t& res = m_resources[v * m_sizeU + u];
    if (res != amount)
        res = amount;
}

uint32_t Map::getResource(uint32_t const u, uint32_t const v)
{
    return m_resources[v * m_sizeU + u];
}

uint32_t Map::getResource(uint32_t const u, uint32_t const v, uint32_t radius)
{
    uint32_t totalInsideRadius = 0u;
    uint32_t x = u; uint32_t y = v;
    m_coordinates.init(radius, x, y, 0u, m_sizeU, 0u, m_sizeV, false);

    while (m_coordinates.next(x, y))
        totalInsideRadius += getResource(x, y);

    return totalInsideRadius;
}

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

void Map::addResource(uint32_t const u, uint32_t const v, uint32_t const radius, uint32_t toAdd)
{
    uint32_t remainingToAdd = toAdd;
    uint32_t x = u; uint32_t y = v;

    m_coordinates.init(radius, x, y, 0, m_sizeU, 0, m_sizeV, true);
    while ((remainingToAdd > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(u, v);
        toAdd = std::min(m_type.capacity - amount, remainingToAdd);
        if (toAdd > 0u)
        {
            amount += toAdd;
            remainingToAdd -= toAdd;
            setResource(x, y, amount);
        }
    }
}

void Map::removeResource(uint32_t const u, uint32_t const v, uint32_t toRemove)
{
    uint32_t amount = getResource(u, v);

    if (amount > toRemove)
        amount -= toRemove;
    else
        amount = 0u;

    setResource(u, v, amount);
}

void Map::removeResource(uint32_t const u, uint32_t const v, uint32_t radius, uint32_t toRemove)
{
    uint32_t remainingToRemove = toRemove;
    uint32_t x = u; uint32_t y = v;

    m_coordinates.init(radius, x, y, 0u, m_sizeU, 0u, m_sizeV, true);
    while ((remainingToRemove > 0u) && m_coordinates.next(x, y))
    {
        uint32_t amount = getResource(x, y);
        toRemove = std::min(amount, remainingToRemove);
        if (toRemove > 0u)
        {
            amount -= toRemove;
            remainingToRemove -= toRemove;
            setResource(x, y, amount);
        }
    }
}

Vector3f Map::getWorldPosition(uint32_t const u, uint32_t const v)
{
    return Vector3f(float(clamp(u, 0u, m_sizeU)) * config::GRID_SIZE,
                    float(clamp(v, 0u, m_sizeV)) * config::GRID_SIZE,
                    0.0f);
}

// TODO
void Map::executeRules()
{
#if 0
    m_ticks++;

    size_t i = m_rules.size();
    while (i--)
    {
        RuleMap& rule = *(m_rules[i]);

        if (m_ticks % rule.rate == 0u)
            continue;

        if (rule.randomTiles)
        {
            uint32_t tilesAmount = (rule.randomTilesPercent * m_sizeU * m_sizeV) / 100;

            RandomCoordinates r(m_sizeU, m_sizeV);

            for (uint32_t j = 0u; j < tilesAmount; j++)
                if (r.next(&context.mapPositionX, &context.mapPositionY))
                    rule.Execute(context);
        }
        else
        {
            for (uint32_t x = 0u; x < m_sizeU; ++x)
            {
                context.mapPositionX = x;

                for (uint32_t y = 0u; y < m_sizeV; ++y)
                {
                    context.mapPositionY = y;
                    rule.Execute(context);
                }
            }
        }
    }
#endif
}
