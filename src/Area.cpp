//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Area.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/World.hpp"

#include <array>
#include <cmath>
#include <limits>

// -----------------------------------------------------------------------------
namespace ogb {

Area::Area(uint32_t id, AreaType const& type, MapRegion const& footprint, City& city)
    : m_id(id), m_type(type), m_footprint(footprint), m_city(city)
{
    m_context.area = this;
    m_context.city = &city;
    m_context.globals = &(city.globals());
    m_context.clock = &city.world().clock();
    m_context.u = footprint.u0;
    m_context.v = footprint.v0;
}

// -----------------------------------------------------------------------------
void Area::executeRules()
{
    m_ticks += 1u;
    m_context.clock = &m_city.world().clock();

    uint32_t const perMinute = m_city.world().clock().ticksPerMinute();

    size_t i = m_type.rules.size();
    while (i--)
    {
        if (m_ticks % m_type.rules[i]->periodTicks(perMinute) == 0u)
            m_type.rules[i]->execute(m_context);
    }
}

// -----------------------------------------------------------------------------
uint32_t Area::countUnits(std::string const& unitType) const
{
    return uint32_t(unitsInside(unitType).size());
}

// -----------------------------------------------------------------------------
std::vector<Unit*> Area::unitsInside(std::string const& unitType) const
{
    std::vector<Unit*> found;
    for (auto& it: m_city.units())
    {
        if (!unitType.empty() && (it->type() != unitType))
            continue;
        if (m_footprint.contains(it->mapU(), it->mapV()))
            found.push_back(it.get());
    }
    return found;
}

// -----------------------------------------------------------------------------
Vector3f Area::cellWorldPosition(int32_t u, int32_t v) const
{
    Vector3f const topLeft = m_city.world().mapPosition2world(u, v);
    float const size = m_city.gridCellSize();
    return Vector3f(topLeft.x + size * 0.5f, topLeft.y + size * 0.5f, 0.0f);
}

// -----------------------------------------------------------------------------
Way* Area::nearestWay(Vector3f const& world, float& offset,
                      float maxDistance) const
{
    Way* best = nullptr;
    float bestDist = (maxDistance < 0.0f)
                     ? std::numeric_limits<float>::infinity()
                     : (maxDistance * maxDistance);
    offset = 0.5f;

    for (auto& pathIt: m_city.paths())
    {
        for (auto& way: pathIt.second->ways())
        {
            Vector3f const a = way->position1();
            Vector3f const b = way->position2();
            Vector3f const ab = b - a;
            float const length2 = squaredMagnitude(ab);
            float t = 0.5f;
            if (length2 > 1e-8f)
            {
                t = ((world.x - a.x) * ab.x + (world.y - a.y) * ab.y) / length2;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
            }
            Vector3f const projected = a + ab * t;
            float const dist = squaredMagnitude(world - projected);
            if (dist < bestDist)
            {
                bestDist = dist;
                best = way.get();
                offset = t;
            }
        }
    }

    return best;
}

// -----------------------------------------------------------------------------
bool Area::findFreeCell(int32_t& u, int32_t& v) const
{
    if (m_footprint.empty())
        return false;

    std::vector<Unit*> const occupied = unitsInside(std::string());

    auto taken = [&](int32_t cu, int32_t cv) {
        for (Unit* unit: occupied)
        {
            if ((unit->mapU() == cu) && (unit->mapV() == cv))
                return true;
        }
        return false;
    };

    // Walk the footprint from the centre so that the zone fills inward.
    int32_t const cu = m_footprint.u0 + int32_t(m_footprint.sizeU / 2u);
    int32_t const cv = m_footprint.v0 + int32_t(m_footprint.sizeV / 2u);

    int32_t bestU = m_footprint.u0;
    int32_t bestV = m_footprint.v0;
    int32_t bestDist = std::numeric_limits<int32_t>::max();
    bool found = false;

    for (int32_t iu = m_footprint.u0; iu < m_footprint.u1(); ++iu)
    {
        for (int32_t iv = m_footprint.v0; iv < m_footprint.v1(); ++iv)
        {
            if (taken(iu, iv))
                continue;
            int32_t const du = iu - cu;
            int32_t const dv = iv - cv;
            int32_t const dist = du * du + dv * dv;
            if (dist < bestDist)
            {
                bestDist = dist;
                bestU = iu;
                bestV = iv;
                found = true;
            }
        }
    }

    if (!found)
        return false;

    u = bestU;
    v = bestV;
    return true;
}

// -----------------------------------------------------------------------------
bool Area::findBuildableCell(int32_t& u, int32_t& v) const
{
    if (m_footprint.empty())
        return false;

    if (m_city.paths().empty())
        return false;

    std::vector<Unit*> const occupied = unitsInside(std::string());

    auto taken = [&](int32_t cu, int32_t cv) {
        for (Unit* unit: occupied)
        {
            if ((unit->mapU() == cu) && (unit->mapV() == cv))
                return true;
        }
        return false;
    };

    // The cell the road runs through, then the four it fronts.
    static std::array<int32_t, 5u> const NEIGHBOURS_U = { 0, 1, -1, 0, 0 };
    static std::array<int32_t, 5u> const NEIGHBOURS_V = { 0, 0, 0, 1, -1 };

    float const side = m_city.gridCellSize();

    for (auto const& pathIt: m_city.paths())
    {
        for (auto const& way: pathIt.second->ways())
        {
            Vector3f const a = way->position1();
            Vector3f const ab = way->position2() - a;
            // One sample per cell crossed, so that no cell along the segment is
            // missed and none is visited twice.
            int32_t const steps =
                std::max(1, int32_t(std::sqrt(squaredMagnitude(ab)) / side));

            for (int32_t step = 0; step <= steps; ++step)
            {
                Vector3f const point = a + ab * (float(step) / float(steps));
                int32_t cu = 0;
                int32_t cv = 0;
                m_city.world().world2mapPosition(point, cu, cv);

                for (size_t k = 0u; k < NEIGHBOURS_U.size(); ++k)
                {
                    int32_t const su = cu + NEIGHBOURS_U[k];
                    int32_t const sv = cv + NEIGHBOURS_V[k];
                    if (!m_footprint.contains(su, sv))
                        continue;
                    if (taken(su, sv))
                        continue;

                    u = su;
                    v = sv;
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace ogb
