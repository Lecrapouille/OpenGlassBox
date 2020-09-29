#ifndef UNIT_HPP
#  define UNIT_HPP

#  include "Core/Path.hpp"
#  include "Core/Rule.hpp"
#  include <algorithm>

class RuleUnit;
class City;

//==============================================================================
//! \brief A Unit represents things: houses, factories, even people.
//! A unit has state: a collection of resource.
//! But also a well-defined spatial extent: bounding volume, simulation
//! footprint.
//==============================================================================
class Unit
{
public:

    // -------------------------------------------------------------------------
    //! \brief
    //! TODO const& type ?
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Node& node, City& city);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    bool accepts(std::string const& searchTarget,
                 Resources const& resourcesToTryToAdd);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline std::string const& name() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Resources& resources() { return m_resources; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Vector3f const& position() const { return m_node.position(); }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Node& node() { return m_node; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Unit.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_type.color; }

private:

    UnitType const& m_type;
    Node          & m_node;
    Resources       m_resources;
    RuleContext     m_context;
    uint32_t        m_ticks = 0u;
};

using Units = std::vector<std::unique_ptr<Unit>>;

#endif
