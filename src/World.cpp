//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/World.hpp"

#include <cmath>
#include <stdexcept>

// -----------------------------------------------------------------------------
namespace ogb
{

World::World(Config const& config, SimulationClock& clock)
    : m_config(config), m_clock(clock)
{
}

// -----------------------------------------------------------------------------
void World::setConfig(Config const& config)
{
    m_config = config;
    m_clock.setTicksPerMinute(m_config.time.ticksPerMinute);
}

// -----------------------------------------------------------------------------
Layer& World::addLayer(LayerType const& type)
{
    auto const it = m_layers.find(type.name.str());
    if (it != m_layers.end())
        return *(it->second);

    return *(m_layers[type.name.str()] = std::make_unique<Layer>(type, *this));
}

// -----------------------------------------------------------------------------
Layer* World::findLayer(std::string const& name)
{
    auto const it = m_layers.find(name);
    return (it == m_layers.end()) ? nullptr : it->second.get();
}

// -----------------------------------------------------------------------------
City& World::addCity(std::string const& name,
                     Vector3f const& position,
                     uint32_t sizeU,
                     uint32_t sizeV)
{
    City& city = *(m_cities[name] = std::make_unique<City>(
                       name, position, sizeU, sizeV, *this));
    m_listener->onCityAdded(city);
    return city;
}

// -----------------------------------------------------------------------------
City& World::addCity(std::string const& name, Vector3f const& position)
{
    return addCity(name,
                   position,
                   m_config.grid.defaultCitySizeU,
                   m_config.grid.defaultCitySizeV);
}

// -----------------------------------------------------------------------------
bool World::removeCity(std::string const& name)
{
    auto const it = m_cities.find(name);
    if (it == m_cities.end())
        return false;

    // Told while the city is still alive: a renderer has to drop what it drew
    // before the buildings it refers to are destroyed.
    m_listener->onCityRemoved(*it->second);
    m_cities.erase(it);
    return true;
}

// -----------------------------------------------------------------------------
City& World::getCity(std::string const& name)
{
    auto const it = m_cities.find(name);
    if (it == m_cities.end())
        throw std::out_of_range("Unknown city '" + name + "'");
    return *it->second;
}

// -----------------------------------------------------------------------------
City* World::findCity(std::string const& name)
{
    auto const it = m_cities.find(name);
    return (it == m_cities.end()) ? nullptr : it->second.get();
}

// -----------------------------------------------------------------------------
void World::update(float dt)
{
    m_clock.setTicksPerMinute(m_config.time.ticksPerMinute);
    m_clock.tick();

    for (auto const& [_, city] : m_cities)
    {
        city->update(dt);
    }

    // Layers are shared, so their rules run once for the whole world rather
    // than once per city.
    for (auto const& [_, layer] : m_layers)
    {
        layer->executeRules(m_cities);
    }
}

// -----------------------------------------------------------------------------
Cell World::worldToCell(Vector3f const& position) const
{
    float const size = m_config.grid.cellSize;

    return Cell{ int32_t(std::floor(position.x / size)),
                 int32_t(std::floor(position.y / size)) };
}

// -----------------------------------------------------------------------------
Vector3f World::cellToWorld(Cell cell) const
{
    float const size = m_config.grid.cellSize;

    return Vector3f(float(cell.u) * size, float(cell.v) * size, 0.0f);
}

// -----------------------------------------------------------------------------
City* World::findCityAt(Vector3f const& position)
{
    Cell const cell = worldToCell(position);
    for (auto const& [cityName, city] : m_cities)
    {
        (void)cityName;
        if (city->getRegion().contains(cell))
            return city.get();
    }
    return nullptr;
}

namespace
{

bool clipToBox(Vector3f const& a,
               Vector3f const& b,
               float x0,
               float y0,
               float x1,
               float y1,
               float& tEnter,
               float& tLeave)
{
    float t0 = 0.0f;
    float t1 = 1.0f;
    float const dx = b.x - a.x;
    float const dy = b.y - a.y;

    auto const clip = [&](float p, float q) -> bool
    {
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

    if (!clip(-dx, a.x - x0) || !clip(dx, x1 - a.x) || !clip(-dy, a.y - y0) ||
        !clip(dy, y1 - a.y))
    {
        return false;
    }

    tEnter = t0;
    tLeave = t1;
    return tLeave > tEnter;
}

Node& ensureNode(Path& path, Vector3f const& position)
{
    for (auto& node : path.getNodes())
    {
        if (length(node->getPosition() - position) < 1.5f)
            return *node;
    }
    return path.addNode(position);
}

} // namespace

// -----------------------------------------------------------------------------
bool World::addRoad(City& owner,
                    std::string const& pathType,
                    SegmentType const& segmentType,
                    Vector3f const& from,
                    Vector3f const& to) const
{
    Listener::SegmentProposal const proposal{ from,
                                              to,
                                              segmentType.name.str() };

    std::vector<RoadPiece> pieces;
    if (!cutRoadPerCity(owner, from, to, proposal, pieces))
    {
        // A neighbour refused its share, and a road laid only on this side of
        // the border is not the road that was asked for.
        return false;
    }

    if (pieces.empty())
    {
        // Entirely outside every city: still give it to the requester.
        pieces.push_back({ &owner, from, to });
    }

    for (RoadPiece const& piece : pieces)
    {
        layRoadPiece(piece, owner, pathType, segmentType);
    }

    return true;
}

// -----------------------------------------------------------------------------
bool World::cutRoadPerCity(City& owner,
                           Vector3f const& from,
                           Vector3f const& to,
                           Listener::SegmentProposal const& proposal,
                           std::vector<RoadPiece>& pieces) const
{
    for (auto& it : m_cities) // NOSONAR
    {
        City& city = *it.second;

        // Keep the part of the road that falls inside the cells of that city,
        // as two positions along the line rather than as a clipped copy of it.
        CellRegion const region = city.getRegion();
        Vector3f const p0 = cellToWorld(Cell{ region.u0, region.v0 });
        Vector3f const p1 =
            cellToWorld(Cell{ region.getMaxU(), region.getMaxV() });
        float t0 = 0.0f;
        float t1 = 1.0f;
        if (!clipToBox(from, to, p0.x, p0.y, p1.x, p1.y, t0, t1))
            continue;

        // A road that only grazes a corner of the city is not worth a segment
        // of its own there.
        Vector3f const a = from + (to - from) * t0;
        Vector3f const b = from + (to - from) * t1;
        if (length(b - a) < 1e-3f)
            continue;

        // Building on someone else's ground is theirs to refuse.
        if ((&city != &owner) &&
            !m_listener->allowSegmentAcross(owner, city, proposal))
        {
            return false;
        }

        pieces.push_back({ &city, a, b });
    }

    return true;
}

// -----------------------------------------------------------------------------
void World::layRoadPiece(RoadPiece const& piece,
                         City const& owner,
                         std::string const& pathType,
                         SegmentType const& segmentType)
{
    // The city may already have that network, or it has to be given one of the
    // same kind as the requester's. A requester without one has nothing to copy
    // from, and the piece is dropped.
    Path* path = nullptr;
    auto const found = piece.city->getPaths().find(pathType);
    if (found != piece.city->getPaths().end())
    {
        path = found->second.get();
    }
    else
    {
        auto const ownerPath = owner.getPaths().find(pathType);
        if (ownerPath == owner.getPaths().end())
            return;
        path = &piece.city->addPath(ownerPath->second->getType());
    }

    // Both ends join whatever crossroads already stands there, so two roads
    // drawn to the same corner meet instead of passing through one another.
    Node& n1 = ensureNode(*path, piece.a);
    Node& n2 = ensureNode(*path, piece.b);
    if (&n1 != &n2)
        path->addSegment(segmentType, n1, n2);
}

} // namespace ogb
