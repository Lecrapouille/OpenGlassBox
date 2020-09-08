#include "Map.hpp"

const float Map::GRID_SIZE = 2.0f;
const uint32_t Map::MAX_CAPACITY = std::numeric_limits<uint32_t>::max();

template<typename T>
static inline T clamp(T const value, T const lower, T const upper)
{
    if (value < lower)
        return lower;

    if (value > upper)
        return upper;

    return value;
}

Map::Map(std::string const& id, uint32_t sizeU, uint32_t sizeV, uint32_t capacity)
    : m_id(id),
      m_sizeU(sizeU), m_sizeV(sizeV),
      m_capacity(capacity),
      m_resources(sizeU * sizeV, 0u)
{}

void Map::setResource(uint32_t const u, uint32_t const v, uint32_t amount)
{
    if (amount > m_capacity)
        amount = m_capacity;

    if (m_resources[v * m_sizeU + u] != amount)
    {
        m_resources[v * m_sizeU + u] = amount;
        //mapListener->OnMapModified(*this, u, v, amount);
    }
}

uint32_t Map::getResource(uint32_t const u, uint32_t const v)
{
    return m_resources[v * m_sizeU + u];
}

#if 0 // TODO
uint32_t Map::getResource(uint32_t const u, uint32_t const v, uint32_t radius)
{
    uint32_t totalInsideRadius = 0u;
    CoordinatesInsideRadius c(radius, u, v, 0u, m_sizeU, 0u, m_sizeV, false);

    while (c.GetNextCoordinate(&u, &v))
        totalInsideRadius += getResource(u, v);

    return totalInsideRadius;
}

uint32_t Map::getCapacity(uint32_t const u, uint32_t const v)
{
    return m_capacity; // FIXME
}

uint32_t Map::getCapacity(uint32_t const u, uint32_t const v, uint32_t const radius)
{
    uint32_t capacityInsideRadius = 0u;
    CoordinatesInsideRadius c(radius, u, v, 0u, m_sizeU, 0u, m_sizeV, false);

    while (c.GetNextCoordinate(&u, &v))
        capacityInsideRadius += getCapacity(u, v);

    return capacityInsideRadius;
}
#endif

void Map::addResource(uint32_t const u, uint32_t const v, uint32_t toAdd)
{
    uint32_t amount = getResource(u, v);

    // Avoid integer overflow
    if (amount >= Map::MAX_CAPACITY - toAdd)
        amount = Map::MAX_CAPACITY;
    else
        amount += toAdd;

    setResource(u, v, amount);
}

#if 0 // TODO
void Map::addResource(uint32_t const u, uint32_t const v, uint32_t const radius, uint32_t const toAdd)
{
    CoordinatesInsideRadius c(radius, u, v, 0, m_sizeU, 0, m_sizeV, true);

    uint32_t remainingToAdd = toAdd;
    while ((remainingToAdd > 0u) && c.GetNextCoordinate(&u, &v))
    {
        uint32_t amount = getResource(u, v);

        toAdd = std::min(m_capacity - amount, remainingToAdd);
        if (toAdd > 0)
        {
            amount += toAdd;
            remainingToAdd -= toAdd;
            setResource(u, v, amount);
        }
    }
}
#endif

void Map::removeResource(uint32_t const u, uint32_t const v, uint32_t toRemove)
{
    uint32_t amount = getResource(u, v);

    if (amount > toRemove)
        amount -= toRemove;
    else
        amount = 0u;

    setResource(u, v, amount);
}

#if 0 // TODO
void Map::removeResource(uint32_t const u, uint32_t const v, uint32_t radius, uint32_t toRemove)
{
    uint32_t remainingToRemove = toRemove;
    CoordinatesInsideRadius c(radius, u, v, 0u, m_sizeU, 0u, m_sizeV, true);

    while ((remainingToRemove > 0u) && c.GetNextCoordinate(&u, &v))
    {
        uint32_t amount = getResource(u, v);
        uint32_ toRemove = std::min(amount, remainingToRemove);
        if (toRemove > 0u)
        {
            amount -= toRemove;
            remainingToRemove -= toRemove;
            setResource(u, v, amount);
        }
    }
}
#endif

Vector3f Map::getWorldPosition(uint32_t const u, uint32_t const v)
{
    return Vector3f(float(clamp(u, 0u, m_sizeU)) * GRID_SIZE,
                    float(clamp(v, 0u, m_sizeV)) * GRID_SIZE,
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
        SimRuleMap& rule = *(m_rules[i]);

        if (m_ticks % rule.rate == 0u)
            continue;

        if (rule.randomTiles)
        {
            uint32_t tilesAmount = (rule.randomTilesPercent * m_sizeU * m_sizeV) / 100;

            RandomCoordinates r(m_sizeU, m_sizeV);

            for (uint32_t j = 0u; j < tilesAmount; j++)
                if (r.GetNextCoordinate(&context.mapPositionX, &context.mapPositionY))
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
