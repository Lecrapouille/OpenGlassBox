//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/World.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace ogb
{

// -----------------------------------------------------------------------------
Zone::Zone(size_t id,
           ZoneType const& type,
           CellRegion const& footprint,
           City& city)
    : m_id(id), m_type(type), m_footprint(footprint), m_city(city)
{
    m_rule_context.zone = this;
    m_rule_context.city = &city;
    m_rule_context.globals = &(city.getGlobals());
    m_rule_context.clock = &city.getClock();
    m_rule_context.cell = { footprint.u0, footprint.v0 };
}

// -----------------------------------------------------------------------------
Cell Zone::getRuleCell() const
{
    // A zone rule asks questions about a place: is there water here, is the air
    // clean enough for a better house. The place that answers them is the plot
    // the next building would stand on, so the rules read the layers there.
    //
    // The corner of the footprint used to answer instead, which is an arbitrary
    // cell: a zone of twelve by twelve decided whether it could grow from what
    // its cell (0,0) held, however far that was from any road.
    std::optional<Cell> const plot = findFreeCellNearRoad();
    if (plot.has_value())
        return *plot;

    // A full zone has no plot left. Its rules are the ones that upgrade and
    // demolish, and the centre is the cell that best represents the whole.
    return Cell{ m_footprint.u0 + int32_t(m_footprint.sizeU / 2u),
                 m_footprint.v0 + int32_t(m_footprint.sizeV / 2u) };
}

// -----------------------------------------------------------------------------
void Zone::executeRules()
{
    m_ticks += 1u;
    m_rule_context.clock = &m_city.getClock();

    uint32_t const perMinute = m_city.getClock().getTicksPerMinute();

    // Locating the plot walks the road network, so it happens once for the
    // whole zone and only on the ticks where a rule actually runs. Periods are
    // written in hours or in days, so that is rare.
    bool located = false;

    size_t i = m_type.rules.size();
    while (i--)
    {
        if (m_ticks % m_type.rules[i]->getPeriodTicks(perMinute) != 0u)
            continue;

        if (!located)
        {
            m_rule_context.cell = getRuleCell();
            located = true;
        }

        m_type.rules[i]->execute(m_rule_context);
    }
}

// -----------------------------------------------------------------------------
uint32_t Zone::countBuildings(Name const& buildingType) const
{
    return uint32_t(getBuildingsInside(buildingType).size());
}

// -----------------------------------------------------------------------------
std::vector<Building*> Zone::getBuildingsInside(Name const& buildingType) const
{
    std::vector<Building*> found;
    for (auto const& building : m_city.getBuildings())
    {
        if (!buildingType.empty() && (building->getTypeName() != buildingType))
            continue;
        if (m_footprint.contains(building->getCell()))
            found.push_back(building.get());
    }
    return found;
}

// -----------------------------------------------------------------------------
Vector3f Zone::getCellCentre(Cell cell) const
{
    Vector3f const topLeft = m_city.cellToWorld(cell);
    float const half = m_city.getCellSize() * 0.5f;
    return { topLeft.x + half, topLeft.y + half, 0.0f };
}

// -----------------------------------------------------------------------------
Segment* Zone::findNearestSegment(Vector3f const& position,
                                  float& offset,
                                  float maxDistance) const
{
    Segment* best = nullptr;
    float bestDist =
        (maxDistance < 0.0f) ? ROUTING_INFINITY : (maxDistance * maxDistance);
    offset = 0.5f;

    for (auto const& [pathName, path] : m_city.getPaths())
    {
        (void)pathName;
        for (auto const& segment : path->getSegments())
        {
            Vector3f const a = segment->getFromPosition();
            Vector3f const b = segment->getToPosition();
            Vector3f const ab = b - a;
            float const length2 = lengthSquared(ab);
            float t = 0.5f;
            if (length2 > 1e-8f)
            {
                t = ((position.x - a.x) * ab.x + (position.y - a.y) * ab.y) /
                    length2;
                if (t < 0.0f)
                    t = 0.0f;
                if (t > 1.0f)
                    t = 1.0f;
            }
            Vector3f const projected = a + ab * t;
            float const dist = lengthSquared(position - projected);
            if (dist < bestDist)
            {
                bestDist = dist;
                best = segment.get();
                offset = t;
            }
        }
    }

    return best;
}

// -----------------------------------------------------------------------------
std::optional<Cell> Zone::findFreeCell() const
{
    if (m_footprint.isEmpty())
        return {};

    std::vector<Building*> const occupied = getBuildingsInside(Name());

    auto taken = [&](Cell cell)
    {
        for (Building const* building : occupied)
        {
            if (building->getCell() == cell)
                return true;
        }
        return false;
    };

    // Walk the footprint from the centre so that the zone fills inward.
    Cell const centre{ m_footprint.u0 + int32_t(m_footprint.sizeU / 2u),
                       m_footprint.v0 + int32_t(m_footprint.sizeV / 2u) };

    std::optional<Cell> best;
    int32_t bestDist = std::numeric_limits<int32_t>::max();

    for (int32_t u = m_footprint.u0; u < m_footprint.getMaxU(); ++u)
    {
        for (int32_t v = m_footprint.v0; v < m_footprint.getMaxV(); ++v)
        {
            Cell const cell{ u, v };
            if (taken(cell))
                continue;
            int32_t const du = u - centre.u;
            int32_t const dv = v - centre.v;
            int32_t const dist = du * du + dv * dv;
            if (dist < bestDist)
            {
                bestDist = dist;
                best = cell;
            }
        }
    }

    return best;
}

// -----------------------------------------------------------------------------
std::optional<Cell> Zone::findFreeCellNearRoad() const
{
    // An empty zone has no plot, and a city with no street has nothing to build
    // along: a building nobody can reach is worse than no building at all.
    if (m_footprint.isEmpty() || m_city.getPaths().empty())
        return {};

    // Reading the whole zone once is cheaper than asking the city about every
    // plot the walk below looks at.
    std::vector<Building*> const occupied = getBuildingsInside(Name());

    // Walk the streets rather than the cells: a zone can be much larger than
    // the part of it a road actually reaches.
    for (auto const& [pathName, path] : m_city.getPaths())
    {
        (void)pathName;
        for (auto const& segment : path->getSegments())
        {
            std::optional<Cell> const cell =
                freeCellAlongSegment(*segment, occupied);
            if (cell.has_value())
                return cell;
        }
    }

    return {};
}

// -----------------------------------------------------------------------------
std::optional<Cell>
Zone::freeCellAlongSegment(Segment const& segment,
                           std::vector<Building*> const& occupied) const
{
    Vector3f const a = segment.getFromPosition();
    Vector3f const ab = segment.getToPosition() - a;

    // One sample per cell crossed, so that no cell along the segment is
    // missed and none is visited twice.
    float const side = m_city.getCellSize();
    int32_t const steps =
        std::max(1, int32_t(std::sqrt(lengthSquared(ab)) / side));

    for (int32_t step = 0; step <= steps; ++step)
    {
        Vector3f const point = a + ab * (float(step) / float(steps));
        std::optional<Cell> const cell =
            freeCellAround(m_city.worldToCell(point), occupied);
        if (cell.has_value())
            return cell;
    }

    return {};
}

// -----------------------------------------------------------------------------
std::optional<Cell>
Zone::freeCellAround(Cell const& crossed,
                     std::vector<Building*> const& occupied) const
{
    // The cell the road runs through, then the four it fronts.
    static std::array<Cell, 5u> const NEIGHBOURS = {
        Cell{ 0, 0 }, Cell{ 1, 0 }, Cell{ -1, 0 }, Cell{ 0, 1 }, Cell{ 0, -1 }
    };

    for (Cell const& neighbour : NEIGHBOURS)
    {
        Cell const cell{ crossed.u + neighbour.u, crossed.v + neighbour.v };

        // The street may run outside the zone, or past a plot already built on.
        if (!m_footprint.contains(cell))
            continue;

        bool taken = false;
        for (Building const* building : occupied)
        {
            if (building->getCell() == cell)
            {
                taken = true;
                break;
            }
        }

        if (!taken)
            return cell;
    }

    return {};
}

} // namespace ogb
