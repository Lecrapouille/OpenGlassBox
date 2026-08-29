//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Zone.hpp
//! \brief Rectangular zones painted on the cell grid, with optional zone rules.

#ifndef OPEN_GLASSBOX_ZONE_HPP
#define OPEN_GLASSBOX_ZONE_HPP

#include "OpenGlassBox/CellRegion.hpp"
#include "OpenGlassBox/Rule.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <memory>
#include <optional>

namespace ogb
{

class City;
class Unit;
class Segment;

//==============================================================================
//! \brief A zone the player painted, which grows, upgrades and demolishes
//! buildings on its own.
//!
//! This is the piece that turns an empty grid into a city. The player paints a
//! residential or an industrial rectangle and the simulation fills it, one
//! building at a time, as long as its rules are satisfied: enough demand, a
//! road within reach, a free cell.
//!
//! A Zone owns its footprint and nothing else. The buildings it grew belong to
//! the City, which is why it counts them by walking the City instead of keeping
//! a list: buildings come and go, and a stale pointer would outlive the thing
//! it points at.
//!
//! Example:
//! \code
//! // A residential zone of ten by ten cells, and what it has grown so far.
//! Zone& zone = city.addZone(rules.getZoneType("Residential"),
//!                           CellRegion{ 0, 0, 10u, 10u });
//! std::cout << zone.countUnits("Home") << " homes in "
//!           << zone.getTypeName() << '\n';
//! \endcode
//!
//! The matching script. The rule below grows a house at a time, beside a road,
//! until the zone holds ten of them:
//! \code
//! rules
//!     zoneRule GrowHomes
//!         rate 4 hours
//!         count Home less 10
//!         spawn Home at nearestSegment
//!     end
//! end
//!
//! zones
//!     zone Residential color 0x44AA44 rules [ GrowHomes ]
//! end
//! \endcode
//==============================================================================
class Zone
{
public:

    // -------------------------------------------------------------------------
    //! \brief \param[in] id identifier, unique inside the City.
    //! \param[in] type recipe of the zone: its name, its colour and the rules
    //! to run. Kept by reference and has to outlive the Zone.
    //! \param[in] footprint the cells the player painted, on the grid of the
    //! World. Copied.
    //! \param[in] city the city the zone belongs to and grows buildings in.
    //! Kept by reference.
    // -------------------------------------------------------------------------
    Zone(uint32_t id,
         ZoneType const& type,
         CellRegion const& footprint,
         City& city);

    ~Zone() = default;

    // -------------------------------------------------------------------------
    //! \brief Count one tick and run the rules of the type whose rate falls on
    //! it. Called once per tick by the City.
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \brief \return the identifier, unique inside the City. What a save file
    //! writes down and what an undo refers to.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getId() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the name of its type, such as "Residential".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the colour of its type, as 0xRRGGBB, which the demo
    //! shades the painted rectangle with.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the cells the zone covers.
    // -------------------------------------------------------------------------
    [[nodiscard]] CellRegion const& getRegion() const
    {
        return m_footprint;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how many ticks the zone has lived. A rule with a rate of
    //! two hundred fires when this is a multiple of two hundred.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTicks() const
    {
        return m_ticks;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the rules the zone runs, from its type. Read by the demo
    //! to show what a zone is trying to do.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<RuleZone*> const& getRules() const
    {
        return m_type.rules;
    }

    // -------------------------------------------------------------------------
    //! \brief How many buildings of one kind the zone holds. What a
    //! \c count rule compares against.
    //!
    //! The Zone keeps no list of its buildings: they come and go, and walking
    //! the City costs less than keeping pointers that would dangle.
    //!
    //! \param[in] unitType name of the kind to count, such as "Home".
    //! \return how many buildings of that kind stand on a cell of the
    //! footprint.
    // -------------------------------------------------------------------------
    uint32_t countUnits(Name const& unitType) const;

    // -------------------------------------------------------------------------
    //! \brief The buildings the zone holds, which is what a rule demolishing or
    //! upgrading one picks from.
    //! \param[in] unitType name of the kind to look for, or empty for every
    //! kind.
    //! \return pointers to the buildings standing on a cell of the footprint.
    //! Not owned: the City owns them, and the list goes stale as soon as one is
    //! built or demolished.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Unit*> getUnitsInside(Name const& unitType = {}) const;

    // -------------------------------------------------------------------------
    //! \brief \param[in] cell the cell to test, on the grid of the World.
    //! \return true when the cell belongs to the zone.
    // -------------------------------------------------------------------------
    bool contains(Cell cell) const
    {
        return m_footprint.contains(cell);
    }

    // -------------------------------------------------------------------------
    //! \brief \param[in] cell the cell to locate.
    //! \return the world position of its centre, which is where a building
    //! grown on it is put.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f getCellCentre(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \brief \return the city the zone grows buildings in.
    //!
    //! \note Writable: growing a building is exactly what a zone does.
    // -------------------------------------------------------------------------
    [[nodiscard]] City& getCity() const
    {
        return m_city;
    }

    // -------------------------------------------------------------------------
    //! \brief The road nearest a place, which is what a building has to be
    //! served by.
    //!
    //! Used by the \c at \c nearestSegment form of a creation rule: the building is
    //! put where the road is, not where the cell is, and the road is then cut
    //! in two so that the building gets an address.
    //!
    //! \param[in] position the place to look around, in world coordinates.
    //! \param[out] offset where the projection lands along the segment, from 0
    //! at its first end to 1 at the other. Untouched when nothing is found.
    //! \param[in] maxDistance how far the road may be, in world units. A place
    //! further than that from any road is not served by one, and no building is
    //! grown there. Negative means no limit.
    //! \return the segment, or nullptr when there is no road within reach.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findNearestSegment(Vector3f const& position,
                        float& offset,
                        float maxDistance = -1.0f) const;

    // -------------------------------------------------------------------------
    //! \brief A cell of the footprint holding no building at all, whatever its
    //! kind: two buildings never share a cell.
    //! \return the cell, or nothing at all when the zone is full.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::optional<Cell> findFreeCell() const;

    // -------------------------------------------------------------------------
    //! \brief A free cell of the footprint that a road runs through or fronts,
    //! so a building put there is connected to the network.
    //!
    //! The search walks the roads rather than the cells: a large city holds a
    //! few thousand segments against a few hundred thousand cells, and a
    //! building belongs beside the road serving it anyway.
    //!
    //! \return the cell, or nothing at all when no free cell of the zone is
    //! served by a road, which is what keeps an unpaved zone empty.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::optional<Cell> findFreeCellNearRoad() const;

private:

    //! \brief Identifier, unique inside the City.
    uint32_t m_id;
    //! \brief Recipe of the zone, shared with every zone of that kind.
    ZoneType const& m_type;
    //! \brief The cells the player painted.
    CellRegion m_footprint;
    //! \brief The city the zone grows buildings in. Not owned: the City owns
    //! the Zone.
    City& m_city;
    //! \brief Everything a rule needs while it runs. Held here rather than
    //! built on each call.
    RuleContext m_context;
    //! \brief How many ticks the zone has lived, which is what the rate of a
    //! rule is counted against.
    uint32_t m_ticks = 0u;
};

//! \brief The zones of a City, which owns them.
using Zones = std::vector<std::unique_ptr<Zone>>;

} // namespace ogb

#endif
