//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/World.hpp"

#include <cmath>

// -----------------------------------------------------------------------------
namespace ogb {

World::World(SimulationConfig const& config)
    : m_config(config),
      m_clock(config.ticksPerMinute)
{
    // A city that opens at midnight keeps the player waiting until the rules
    // that hold office hours wake up. Loading a save overwrites this, since it
    // restores the tick counter it was written with.
    m_clock.setTimeOfDay(0u, config.startHour, 0u);
}

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

// -----------------------------------------------------------------------------
City* World::cityAt(Vector3f const& world)
{
    int32_t u = 0;
    int32_t v = 0;
    world2mapPosition(world, u, v);
    for (auto& it: m_cities)
    {
        if (it.second->region().contains(u, v))
            return it.second.get();
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
City const* World::cityAt(Vector3f const& world) const
{
    int32_t u = 0;
    int32_t v = 0;
    world2mapPosition(world, u, v);
    for (auto const& it: m_cities)
    {
        if (it.second->region().contains(u, v))
            return it.second.get();
    }
    return nullptr;
}

namespace {

bool clipToBox(Vector3f const& a, Vector3f const& b, float x0, float y0,
               float x1, float y1, float& tEnter, float& tLeave)
{
    float t0 = 0.0f;
    float t1 = 1.0f;
    float const dx = b.x - a.x;
    float const dy = b.y - a.y;

    auto const clip = [&](float p, float q) -> bool {
        if (std::fabs(p) < 1e-8f)
            return q >= 0.0f;
        float const r = q / p;
        if (p < 0.0f)
        {
            if (r > t1)
                return false;
            if (r > t0)
                t0 = r;
        }
        else
        {
            if (r < t0)
                return false;
            if (r < t1)
                t1 = r;
        }
        return true;
    };

    if (!clip(-dx, a.x - x0) || !clip(dx, x1 - a.x) ||
        !clip(-dy, a.y - y0) || !clip(dy, y1 - a.y))
    {
        return false;
    }

    tEnter = t0;
    tLeave = t1;
    return tLeave > tEnter;
}

Node& ensureNode(Path& path, Vector3f const& position)
{
    for (auto& node: path.nodes())
    {
        if (magnitude(node->position() - position) < 1.5f)
            return *node;
    }
    return path.addNode(position);
}

} // namespace

// -----------------------------------------------------------------------------
bool World::addRoad(City& owner, std::string const& pathType, WayType const& wayType,
                    Vector3f const& from, Vector3f const& to)
{
    WayProposal const proposal{ from, to, wayType.name };

    struct Piece
    {
        City* city;
        Vector3f a;
        Vector3f b;
    };
    std::vector<Piece> pieces;

    for (auto& it: m_cities)
    {
        City& city = *it.second;
        MapRegion const region = city.region();
        Vector3f const p0 = mapPosition2world(region.u0, region.v0);
        Vector3f const p1 = mapPosition2world(region.u1(), region.v1());
        float t0 = 0.0f;
        float t1 = 1.0f;
        if (!clipToBox(from, to, p0.x, p0.y, p1.x, p1.y, t0, t1))
            continue;

        Vector3f const a = from + (to - from) * t0;
        Vector3f const b = from + (to - from) * t1;
        if (magnitude(b - a) < 1e-3f)
            continue;

        if ((&city != &owner) &&
            !m_listener->allowWayAcross(owner, city, proposal))
        {
            return false;
        }

        pieces.push_back({ &city, a, b });
    }

    if (pieces.empty())
    {
        // Entirely outside every city: still give it to the requester.
        pieces.push_back({ &owner, from, to });
    }

    for (Piece const& piece: pieces)
    {
        Path* path = nullptr;
        auto const found = piece.city->paths().find(pathType);
        if (found != piece.city->paths().end())
        {
            path = found->second.get();
        }
        else
        {
            auto const ownerPath = owner.paths().find(pathType);
            if (ownerPath == owner.paths().end())
                continue;
            path = &piece.city->addPath(ownerPath->second->pathType());
        }

        Node& n1 = ensureNode(*path, piece.a);
        Node& n2 = ensureNode(*path, piece.b);
        if (&n1 != &n2)
            path->addWay(wayType, n1, n2);
    }

    return true;
}

} // namespace ogb
