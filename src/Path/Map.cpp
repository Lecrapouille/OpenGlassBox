#include "Map.hpp"

const float Map::GRID_SIZE = 2.0f;

Map::Map(uint32_t sizeX, uint32_t sizeY)
    : m_sizeX(sizeX), m_sizeY(sizeY),
      m_resources(sizeX * sizeY, 0u)
{}

void Map::setValue(uint32_t const x, uint32_t const y, uint32_t val)
{
    if (val == 0u)
        return ;

    if (val > m_capacity)
        val = m_capacity;

    if (m_resources[y * m_sizeX + x] != val)
    {
        m_resources[y * m_sizeX + x] = val;
        //mapListener->OnMapModified(*this, x, y, val);
    }
}

uint32_t Map::getValue(uint32_t const x, uint32_t const y)
{
    return m_resources[y * m_sizeX + x];
}

uint32_t Map::getValue(uint32_t const x, uint32_t const y, uint32_t radius)
{
    uint32_t totalInsideRadius = 0u;
    CoordinatesInsideRadius c(radius, x, y, 0u, m_sizeX, 0u, m_sizeY, false);

    while (c.GetNextCoordinate(&x, &y))
        totalInsideRadius += getValue(x, y);

    return totalInsideRadius;
}

uint32_t Map::getCapacity(uint32_t const x, uint32_t const y)
{
    return m_capacity; // FIXME
}

uint32_t Map::getCapacity(uint32_t const x, uint32_t const y, uint32_t const radius)
{
    uint32_t capacityInsideRadius = 0u;
    CoordinatesInsideRadius c(radius, x, y, 0u, m_sizeX, 0u, m_sizeY, false);

    while (c.GetNextCoordinate(&x, &y))
        capacityInsideRadius += getCapacity(x, y);

    return capacityInsideRadius;
}

void Map::add(uint32_t const x, uint32_t const y, uint32_t const toAdd)
{
    uint32_t val = getValue(x, y);

    toAdd = std::min(m_capacity - val, toAdd);
    if (toAdd > 0u)
    {
        val += toAdd;
        setValue(x, y, val);
    }
}

void Map::add(uint32_t const x, uint32_t const y, uint32_t const radius, uint32_t const toAdd)
{
    CoordinatesInsideRadius c(radius, x, y, 0, m_sizeX, 0, m_sizeY, true);

    uint32_t remainingToAdd = toAdd;
    while ((remainingToAdd > 0u) && c.GetNextCoordinate(&x, &y))
    {
        uint32_t val = getValue(x, y);

        toAdd = std::min(m_capacity - val, remainingToAdd);
        if (toAdd > 0)
        {
            val += toAdd;
            remainingToAdd -= toAdd;
            setValue(x, y, val);
        }
    }
}

void Map::remove(uint32_t const x, uint32_t const y, uint32_t const toRemove)
{
    uint32_t val = getValue(x, y);

    toRemove = std::min(val, toRemove);
    if (toRemove > 0u)
    {
        val -= toRemove;
        setValue(x, y, val);
    }
}

void Map::remove(uint32_t const x, uint32_t const y, uint32_t radius, uint32_t toRemove)
{
    uint32_t remainingToRemove = toRemove;
    CoordinatesInsideRadius c(radius, x, y, 0u, m_sizeX, 0u, m_sizeY, true);

    while ((remainingToRemove > 0u) && c.GetNextCoordinate(&x, &y))
    {
        uint32_t val = getValue(x, y);
        mapType
                toRemove = std::min(val, remainingToRemove);
        if (toRemove > 0)
        {
            val -= toRemove;
            remainingToRemove -= toRemove;
            setValue(x, y, val);
        }
    }
}

Vector3f Map::getWorldPosition(uint32_t const x, uint32_t const y)
{
    return Vector3f(std::max(x, m_sizeX) * GRID_SIZE,
                    0.0,
                    std::max(y, m_sizeY) * GRID_SIZE);
}

void Map::executeRules()
{
    ticks++;

    for (uint32_t i = 0u; i < m_rules.size(); ++i)
    {
        SimRuleMap& rule = *(m_rules[i]);

        if (ticks % rule.rate == 0u)
            continue;

        if (rule.randomTiles)
        {
            uint32_t tilesAmount = (rule.randomTilesPercent * m_sizeX * m_sizeY) / 100;

            RandomCoordinates r(m_sizeX, m_sizeY);

            for (uint32_t j = 0u; j < tilesAmount; j++)
                if (r.GetNextCoordinate(&context.mapPositionX, &context.mapPositionY))
                    rule.Execute(context);
        }
        else
        {
            for (uint32_t x = 0u; x < m_sizeX; ++x)
            {
                context.mapPositionX = x;

                for (uint32_t y = 0u; y < m_sizeY; ++y)
                {
                    context.mapPositionY = y;
                    rule.Execute(context);
                }
            }
        }
    }
}
