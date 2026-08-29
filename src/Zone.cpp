//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/World.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <optional>

// -----------------------------------------------------------------------------
namespace ogb {

Zone::Zone(uint32_t id, ZoneType const& type, CellRegion const& footprint, City& city)
    : m_id(id), m_type(type), m_footprint(footprint), m_city(city)
{
    m_context.zone = this;
    m_context.city = &city;
    m_context.globals = &(city.getGlobals());
    m_context.clock = &city.getClock();
    m_context.cell = { footprint.u0, footprint.v0 };
}

// -----------------------------------------------------------------------------
void Zone::executeRules()
{
    m_ticks += 1u;
    m_context.clock = &m_city.getClock();

    uint32_t const perMinute = m_city.getClock().getTicksPerMinute();

    size_t i = m_type.rules.size();
    while (i--)
    {
        if (m_ticks % m_type.rules[i]->getPeriodTicks(perMinute) == 0u)
            m_type.rules[i]->execute(m_context);
    }
}

// -----------------------------------------------------------------------------
uint32_t Zone::countUnits(Name const& unitType) const
{
    return uint32_t(getUnitsInside(unitType).size());
}

// -----------------------------------------------------------------------------
std::vector<Unit*> Zone::getUnitsInside(Name const& unitType) const
{
    std::vector<Unit*> found;
    for (auto& it: m_city.getUnits())
    {
        if (!unitType.empty() && (it->getTypeName() != unitType))
            continue;
        if (m_footprint.contains(it->getCell()))
            found.push_back(it.get());
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
Segment* Zone::findNearestSegment(Vector3f const& position, float& offset,
                          float maxDistance) const
{
    Segment* best = nullptr;
    float bestDist = (maxDistance < 0.0f)
                     ? ROUTING_INFINITY
                     : (maxDistance * maxDistance);
    offset = 0.5f;

    for (auto& pathIt: m_city.getPaths())
    {
        for (auto& segment: pathIt.second->getSegments())
        {
            Vector3f const a = segment->getFromPosition();
            Vector3f const b = segment->getToPosition();
            Vector3f const ab = b - a;
            float const length2 = lengthSquared(ab);
            float t = 0.5f;
            if (length2 > 1e-8f)
            {
                t = ((position.x - a.x) * ab.x +
                     (position.y - a.y) * ab.y) / length2;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
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

    std::vector<Unit*> const occupied = getUnitsInside(Name());

    auto taken = [&](Cell cell) {
        for (Unit* unit: occupied)
        {
            if (unit->getCell() == cell)
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
    if (m_footprint.isEmpty())
        return {};

    if (m_city.getPaths().empty())
        return {};

    std::vector<Unit*> const occupied = getUnitsInside(Name());

    auto taken = [&](Cell cell) {
        for (Unit* unit: occupied)
        {
            if (unit->getCell() == cell)
                return true;
        }
        return false;
    };

    // The cell the road runs through, then the four it fronts.
    static std::array<Cell, 5u> const NEIGHBOURS = {
        Cell{ 0, 0 }, Cell{ 1, 0 }, Cell{ -1, 0 }, Cell{ 0, 1 }, Cell{ 0, -1 }
    };

    float const side = m_city.getCellSize();

    for (auto const& pathIt: m_city.getPaths())
    {
        for (auto const& segment: pathIt.second->getSegments())
        {
            Vector3f const a = segment->getFromPosition();
            Vector3f const ab = segment->getToPosition() - a;
            // One sample per cell crossed, so that no cell along the segment is
            // missed and none is visited twice.
            int32_t const steps =
                std::max(1, int32_t(std::sqrt(lengthSquared(ab)) / side));

            for (int32_t step = 0; step <= steps; ++step)
            {
                Vector3f const point = a + ab * (float(step) / float(steps));
                Cell const crossed = m_city.worldToCell(point);

                for (Cell const& neighbour: NEIGHBOURS)
                {
                    Cell const cell{ crossed.u + neighbour.u,
                                     crossed.v + neighbour.v };
                    if (!m_footprint.contains(cell))
                        continue;
                    if (taken(cell))
                        continue;

                    return cell;
                }
            }
        }
    }

    return {};
}

} // namespace ogb
