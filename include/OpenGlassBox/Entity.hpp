//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Entity.hpp
//! \brief Shared identity and position for objects a City places on its grid.

#ifndef OPEN_GLASSBOX_ENTITY_HPP
#define OPEN_GLASSBOX_ENTITY_HPP

#include "OpenGlassBox/Name.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <cstddef>

namespace ogb
{

//==============================================================================
//! \brief Base for objects a City places on its grid: id, script type, and position.
//!
//! Building and Agent share the same four fields. Code that reads a city without
//! knowing the object type (renderer, inspector) uses only these fields.
//!
//! \tparam TYPE the script-defined type: BuildingType for a building, AgentType for
//! an Agent. It must have \c name and \c color. It is stored by reference: one
//! type is shared by all entities of that kind. It must outlive them and must
//! not be moved. ScriptDefinitions guarantees this.
//!
//! Node and Segment are not Entity: a Node has no type, and a Segment has no
//! single position.
//!
//! Example:
//! \code
//! // Read a city without knowing the object type.
//! for (auto const& building: city.getBuildings())
//!     std::cout << building->getTypeName() << " #" << building->getId()
//!               << " at " << building->getPosition() << '\n';
//! for (auto const& agent: city.getAgents())
//!     std::cout << agent->getTypeName() << " #" << agent->getId()
//!               << " at " << agent->getPosition() << '\n';
//! \endcode
//==============================================================================
template <class TYPE>
class Entity
{
public:

    //--------------------------------------------------------------------------
    //! \return the id given by the City. Unique among entities of this
    //! kind in that city. Saved to disk and used by undo. Survives a reload.
    //--------------------------------------------------------------------------
    [[nodiscard]] size_t getId() const
    {
        return m_id;
    }

    //--------------------------------------------------------------------------
    //! \return the script type name, such as "Home" or "Truck".
    //! Several entities share one name. This is a kind, not a unique id.
    //!
    //! Interned as a Name: comparison is fast. It still reads and prints as a
    //! string. See Name.
    //--------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    //--------------------------------------------------------------------------
    //! \return the colour from the script for this type, as 0xRRGGBB.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    //--------------------------------------------------------------------------
    //! \return the position in world coordinates.
    //! A Building stays still. An Agent moves every tick.
    //--------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getPosition() const
    {
        return m_position;
    }

protected:

    //--------------------------------------------------------------------------
    //! \param[in] id identifier given by the City.
    //! \param[in] type entity type from the script. Stored by reference. Must
    //! outlive the entity.
    //! \param[in] position position in world coordinates.
    //--------------------------------------------------------------------------
    Entity(size_t id, TYPE const& type, Vector3f const& position)
        : m_id(id), m_type(type), m_position(position)
    {
    }

    //! \brief Id inside the City. Not const: a save restores the saved id.
    size_t m_id;

    //! \brief The type, shared by every entity of the same kind.
    TYPE const& m_type;

    //! \brief Position in world coordinates.
    Vector3f m_position;
};

} // namespace ogb

#endif
