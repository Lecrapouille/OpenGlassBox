//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Entity.hpp
//! \brief Identity and placement shared by the things a City puts on its grid.

#ifndef OPEN_GLASSBOX_ENTITY_HPP
#define OPEN_GLASSBOX_ENTITY_HPP

#include "OpenGlassBox/Name.hpp"
#include "OpenGlassBox/Vector.hpp"

namespace ogb
{

//==============================================================================
//! \brief What a City puts on its grid: a number to be called by, a recipe read
//! from the script, and somewhere to stand.
//!
//! Unit and Agent both answer the same four questions, and used to answer them
//! with their own copy of the same four members. Everything that reads a city
//! without caring what it is looking at, the renderer and the inspector of the
//! demo first of all, only ever asks these.
//!
//! \tparam TYPE the script-defined recipe of the entity: UnitType for a
//! building, AgentType for a traveller. It has to carry a \c name and a
//! \c color, which every EntityType does, and it is held by reference: one
//! recipe is shared by every entity of that kind, so it has to outlive them and
//! must not be moved. ScriptDefinitions is what guarantees that.
//!
//! Node and Segment deliberately stay out of this: a Node has no type and a
//! Segment has no single position, so the base would have to lie about one of
//! them.
//!
//! Example:
//! \code
//! // Reading a city without knowing what is in it.
//! for (auto const& unit: city.getUnits())
//!     std::cout << unit->getTypeName() << " #" << unit->getId()
//!               << " at " << unit->getPosition() << '\n';
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
    //! \brief \return the identifier given by the City, unique among the
    //! entities of that kind inside it. This is what a save file writes down
    //! and what the two ends of an undo refer to, so it survives a reload.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getId() const
    {
        return m_id;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the name of the recipe the script gave, such as "Home" or
    //! "Truck". Several entities share it: this is a kind, not an identity.
    //!
    //! Interned, so asking whether a building is a Home costs an integer
    //! comparison rather than a walk over the characters. It still reads and
    //! prints as a string. See Name.
    //--------------------------------------------------------------------------
    [[nodiscard]] Name const& getTypeName() const
    {
        return m_type.name;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the colour the script chose for that kind, as 0xRRGGBB.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getColor() const
    {
        return m_type.color;
    }

    //--------------------------------------------------------------------------
    //! \brief \return where it stands, in world coordinates. A building stands
    //! still unless the city is moved; an agent moves every tick.
    //--------------------------------------------------------------------------
    [[nodiscard]] Vector3f const& getPosition() const
    {
        return m_position;
    }

protected:

    //--------------------------------------------------------------------------
    //! \brief \param[in] id identifier given by the City.
    //! \param[in] type recipe of the entity. Kept by reference: it has to
    //! outlive the entity.
    //! \param[in] position where it stands, in world coordinates.
    //--------------------------------------------------------------------------
    Entity(uint32_t id, TYPE const& type, Vector3f const& position)
        : m_id(id), m_type(type), m_position(position)
    {
    }

    //! \brief Identifier inside the City. Not const: a save gives back the
    //! numbers it wrote.
    uint32_t m_id;

    //! \brief The recipe, shared with every entity of the same kind.
    TYPE const& m_type;

    //! \brief Where it stands, in world coordinates.
    Vector3f m_position;
};

} // namespace ogb

#endif
