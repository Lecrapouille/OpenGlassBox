//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Zone.hpp
//! \brief Rectangular zones on the cell grid with optional zone rules.

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
class Building;
class Segment;

//==============================================================================
//! \brief A zone the player paints. It grows, upgrades, and demolishes buildings.
//!
//! The player paints a residential or industrial rectangle. The simulation fills
//! it one building at a time when rules pass: enough demand, a road nearby, a
//! free cell.
//!
//! A Zone owns its footprint only. Buildings belong to the City. The Zone counts
//! them by scanning the City. It keeps no list because buildings change often.
//!
//! Example:
//! \code
//! // A residential zone of ten by ten cells, and what it has grown so far.
//! Zone& zone = city.addZone(rules.getZoneType("Residential"),
//!                           CellRegion{ 0, 0, 10u, 10u });
//! std::cout << zone.countBuildings("Home") << " homes in "
//!           << zone.getTypeName() << '\n';
//! \endcode
//!
//! Matching script. The rule grows one house at a time near a road until the
//! zone has ten:
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
    //! \brief Create a zone from a type, footprint, and city.
    //! \param[in] id Unique identifier inside the City.
    //! \param[in] type Zone recipe: name, colour, and rules. Must outlive the Zone.
    //! \param[in] footprint Cells the player painted on the World grid. Copied.
    //! \param[in] city City that owns the zone and holds its buildings. Must outlive
    //! the Zone.
    // -------------------------------------------------------------------------
    Zone(uint32_t id,
         ZoneType const& type,
         CellRegion const& footprint,
         City& city);

    ~Zone() = default;

    // -------------------------------------------------------------------------
    //! \brief Add one tick and run rules whose rate matches. The City calls this
    //! each tick.
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \return Unique identifier inside the City. Used by save files and undo.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getId() const
    {
        return m_id;
    }

    // -------------------------------------------------------------------------
    //! \return Type name, such as "Residential".
    // -------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    // -------------------------------------------------------------------------
    //! \return Type colour as 0xRRGGBB. The demo uses it to shade the rectangle.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    // -------------------------------------------------------------------------
    //! \return Cells the zone covers.
    // -------------------------------------------------------------------------
    [[nodiscard]] CellRegion const& getRegion() const
    {
        return m_footprint;
    }

    // -------------------------------------------------------------------------
    //! \return Tick count. A rule with rate 200 runs when this is a multiple
    //! of 200.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTicks() const
    {
        return m_ticks;
    }

    // -------------------------------------------------------------------------
    //! \return Rules from the zone type. The demo reads them to show zone
    //! behaviour.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<RuleZone*> const& getRules() const
    {
        return m_type.rules;
    }

    // -------------------------------------------------------------------------
    //! \brief Count buildings of one kind in the zone. A \c count rule uses this
    //! value.
    //!
    //! The Zone keeps no building list. It scans the City instead.
    //!
    //! \param[in] buildingType Building kind to count, such as "Home".
    //! \return Count of that kind on footprint cells.
    // -------------------------------------------------------------------------
    uint32_t countBuildings(Name const& buildingType) const;

    // -------------------------------------------------------------------------
    //! \brief List buildings in the zone. Demolish and upgrade rules pick from this
    //! list.
    //! \param[in] buildingType Building kind to find, or empty for all kinds.
    //! \return Pointers to buildings on footprint cells. The City owns them. The list
    //! is invalid after any build or demolish.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Building*> getBuildingsInside(Name const& buildingType = {}) const;

    // -------------------------------------------------------------------------
    //! \brief Test if a cell is inside the zone.
    //! \param[in] cell Cell on the World grid.
    //! \return true if the cell is in the zone.
    // -------------------------------------------------------------------------
    bool contains(Cell cell) const
    {
        return m_footprint.contains(cell);
    }

    // -------------------------------------------------------------------------
    //! \brief World position of a cell centre.
    //! \param[in] cell Cell to locate.
    //! \return World position of the cell centre. New buildings spawn here.
    // -------------------------------------------------------------------------
    [[nodiscard]] Vector3f getCellCentre(Cell cell) const;

    // -------------------------------------------------------------------------
    //! \return City where the zone grows buildings.
    //!
    //! \note Writable. Zones create buildings through this reference.
    // -------------------------------------------------------------------------
    [[nodiscard]] City& getCity() const
    {
        return m_city;
    }

    // -------------------------------------------------------------------------
    //! \brief Find the nearest road to a position. Buildings need a nearby road.
    //!
    //! Used by \c at \c nearestSegment creation rules. The building spawns on the
    //! road. The road splits so the building gets an address.
    //!
    //! \param[in] position Search origin in world coordinates.
    //! \param[out] offset Position along the segment, from 0 to 1. Unchanged if
    //! nothing is found.
    //! \param[in] maxDistance Maximum search distance in world units. Negative means
    //! no limit.
    //! \return The nearest segment, or nullptr if none is in range.
    // -------------------------------------------------------------------------
    [[nodiscard]] Segment* findNearestSegment(Vector3f const& position,
                        float& offset,
                        float maxDistance = -1.0f) const;

    // -------------------------------------------------------------------------
    //! \brief The cell the zone rules read their layers on.
    //!
    //! A zone rule asks whether a place has water, power or clean air. The place
    //! that answers is the plot the next building would stand on, so this returns
    //! findFreeCellNearRoad() when the zone still has room. A full zone returns
    //! the centre of its footprint, since what is left to decide there is which
    //! building to upgrade or to demolish.
    //!
    //! The reach stays at zero cells, so a threshold written in a script means
    //! the same amount whatever the size of the rectangle the player painted.
    //!
    //! \return The cell to read. Always inside the footprint.
    // -------------------------------------------------------------------------
    [[nodiscard]] Cell getRuleCell() const;

    // -------------------------------------------------------------------------
    //! \brief Find an empty footprint cell. Each cell holds at most one building.
    //! \return The cell, or nothing if the zone is full.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::optional<Cell> findFreeCell() const;

    // -------------------------------------------------------------------------
    //! \brief Find a free footprint cell near a road.
    //!
    //! The search walks road segments, not all cells. This is faster in large cities.
    //!
    //! \return The cell, or nothing if no free cell has road access. Unpaved zones
    //! stay empty.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::optional<Cell> findFreeCellNearRoad() const;

private:

    //! \brief Unique identifier inside the City.
    uint32_t m_id;
    //! \brief Zone type recipe, shared by zones of the same kind.
    ZoneType const& m_type;
    //! \brief Cells the player painted.
    CellRegion m_footprint;
    //! \brief City where the zone grows buildings. The City owns the Zone.
    City& m_city;
    //! \brief Rule context. Reused each tick instead of rebuilt.
    RuleContext m_context;
    //! \brief Tick count. Rule rates compare against this value.
    uint32_t m_ticks = 0u;
};

//! \brief Zone list owned by a City.
using Zones = std::vector<std::unique_ptr<Zone>>;

} // namespace ogb

#endif
