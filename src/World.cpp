//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/World.hpp"

#include <cmath>

// -----------------------------------------------------------------------------
World::World(SimulationConfig const& config)
    : m_config(config),
      m_clock(config.ticksPerMinute)
{}

// -----------------------------------------------------------------------------
Map& World::addMap(MapType const& type)
{
    auto const it = m_maps.find(type.name);
    if (it != m_maps.end())
        return *(it->second);

    return *(m_maps[type.name] = std::make_unique<Map>(type, *this));
}

// -----------------------------------------------------------------------------
Map& World::getMap(std::string const& name)
{
    return *m_maps.at(name);
}

// -----------------------------------------------------------------------------
Map const& World::getMap(std::string const& name) const
{
    return *m_maps.at(name);
}

// -----------------------------------------------------------------------------
Map* World::findMap(std::string const& name)
{
    auto const it = m_maps.find(name);
    return (it == m_maps.end()) ? nullptr : it->second.get();
}

// -----------------------------------------------------------------------------
City& World::addCity(std::string const& name, Vector3f const& position,
                     uint32_t sizeU, uint32_t sizeV)
{
    return *(m_cities[name] =
                 std::make_unique<City>(name, position, sizeU, sizeV, *this));
}

// -----------------------------------------------------------------------------
City& World::addCity(std::string const& name, uint32_t sizeU, uint32_t sizeV)
{
    return addCity(name, Vector3f(0.0f, 0.0f, 0.0f), sizeU, sizeV);
}

// -----------------------------------------------------------------------------
City& World::getCity(std::string const& name)
{
    return *m_cities.at(name);
}

// -----------------------------------------------------------------------------
City const& World::getCity(std::string const& name) const
{
    return *m_cities.at(name);
}

// -----------------------------------------------------------------------------
void World::update(float dt)
{
    m_clock.setTicksPerMinute(m_config.ticksPerMinute);
    m_clock.tick();

    for (auto& it: m_cities)
    {
        it.second->update(dt);
    }

    // Maps are shared, so their rules run once for the whole world rather than
    // once per City.
    for (auto& it: m_maps)
    {
        it.second->executeRules(m_cities);
    }
}

// -----------------------------------------------------------------------------
void World::world2mapPosition(Vector3f const& worldPos, int32_t& u,
                              int32_t& v) const
{
    float const size = m_config.gridCellSize;

    u = int32_t(std::floor(worldPos.x / size));
    v = int32_t(std::floor(worldPos.y / size));
}

// -----------------------------------------------------------------------------
Vector3f World::mapPosition2world(int32_t u, int32_t v) const
{
    float const size = m_config.gridCellSize;

    return Vector3f(float(u) * size, float(v) * size, 0.0f);
}
