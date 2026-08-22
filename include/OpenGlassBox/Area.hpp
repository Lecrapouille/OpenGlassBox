//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_AREA_HPP
#  define OPEN_GLASSBOX_AREA_HPP

#  include "OpenGlassBox/MapRegion.hpp"
#  include "OpenGlassBox/Rule.hpp"
#  include "OpenGlassBox/Vector.hpp"
#  include <memory>
#  include <string>
#  include <vector>

namespace ogb {

class City;
class Unit;
class Way;

//==============================================================================
//! \brief A painted zone of the city that creates, upgrades and destroys Units
//! according to its AreaRules.
//!
//! This is the missing piece of GlassBox: the player paints a residential or
//! industrial rectangle, and the simulation fills it. The Area does not own
//! the Units it spawned; the City does. What it owns is the footprint and the
//! rules that walk it.
//==============================================================================
class Area
{
public:

    Area(uint32_t id, AreaType const& type, MapRegion const& footprint, City& city);

    VIRTUAL ~Area() = default;

    // -------------------------------------------------------------------------
    //! \brief Run the rules of this Area whose rate matches the current tick.
    // -------------------------------------------------------------------------
    VIRTUAL void executeRules();

    uint32_t id() const { return m_id; }
    std::string const& type() const { return m_type.name; }
    uint32_t color() const { return m_type.color; }
    MapRegion const& footprint() const { return m_footprint; }
    uint32_t ticks() const { return m_ticks; }
    std::vector<RuleArea*> const& rules() const { return m_type.rules; }

    // -------------------------------------------------------------------------
    //! \brief Number of Units of the given type whose cell sits inside this
    //! Area. The Area does not keep a list of them: Units come and go, and
    //! scanning the City is cheaper than keeping pointers that would dangle.
    // -------------------------------------------------------------------------
    uint32_t countUnits(std::string const& unitType) const;

    // -------------------------------------------------------------------------
    //! \brief Units of the given type (or every Unit when the name is empty)
    //! whose cell sits inside this Area.
    // -------------------------------------------------------------------------
    std::vector<Unit*> unitsInside(std::string const& unitType = {}) const;

    // -------------------------------------------------------------------------
    //! \brief Whether the cell belongs to this Area.
    // -------------------------------------------------------------------------
    bool contains(int32_t u, int32_t v) const { return m_footprint.contains(u, v); }

    // -------------------------------------------------------------------------
    //! \brief World position of the centre of the cell (u, v).
    // -------------------------------------------------------------------------
    Vector3f cellWorldPosition(int32_t u, int32_t v) const;

    City& city() { return m_city; }

    // -------------------------------------------------------------------------
    //! \brief Closest Way of the City to the given world position, or nullptr.
    //! Used by spawn-at-nearestWay.
    // -------------------------------------------------------------------------
    Way* nearestWay(Vector3f const& world, float& offset) const;

    // -------------------------------------------------------------------------
    //! \brief A cell of the footprint that does not already hold a Unit of the
    //! given type. Returns false when the Area is full of them.
    // -------------------------------------------------------------------------
    bool findFreeCell(std::string const& unitType, int32_t& u, int32_t& v) const;

private:

    uint32_t        m_id;
    AreaType const& m_type;
    MapRegion       m_footprint;
    City&           m_city;
    RuleContext     m_context;
    uint32_t        m_ticks = 0u;
};

using Areas = std::vector<std::unique_ptr<Area>>;

} // namespace ogb

#endif
