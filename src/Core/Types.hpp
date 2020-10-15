//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_TYPES_HPP
#  define OPEN_GLASSBOX_TYPES_HPP

#  include "Core/Resources.hpp"

//==========================================================================
//! This file defines all types holding information after the parsing of a
//! simulation script.
//==========================================================================

class RuleUnit;
class RuleMap;
class IRuleCommand;

//==========================================================================
//! \brief Type of RuleMap. Class constructed during the parsing of simulation
//! scripts. Examples:
//!  - map Water remove 10
//!  - map Grass add 1
//==========================================================================
class RuleMapType
{
public:

    RuleMapType(RuleMapType const&) = default;

    RuleMapType(std::string const& name_)
        : name(name_)
    {}

    std::string                name;
    uint32_t                   rate = 1u;
    bool                       randomTiles = false;
    uint32_t                   randomTilesPercent = 10u;
    std::vector<IRuleCommand*> commands; // Shall not be free and pointer stay valid.
};

//==========================================================================
//! \brief Type of RuleMap. Class constructed during the parsing of simulation
//! scripts. Examples:
//!  - map Water remove 10
//!  - map Grass add 1
//==========================================================================
class RuleUnitType
{
public:

    RuleUnitType(RuleUnitType const&) = default;

    RuleUnitType(std::string const& name_)
        : name(name_)
    {}

    std::string                name;
    uint32_t                   rate = 1u;
    RuleUnit*                  onFail = nullptr;
    std::vector<IRuleCommand*> commands; // Shall not be free and pointer stay valid.
};

//==========================================================================
//! \brief Type of Units (Home, Work ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - unit Home color 0xFF00FF radius 1 rules [ SendPeopleToWork ]
//!          targets [ Home ] capacities [ People 4 ] resources [ People 4 ]
//==========================================================================
class UnitType
{
public:

    UnitType(UnitType const&) = default;

    UnitType(std::string const& name_)
        : name(name_), color(0xFFFFFF), radius(1u)
    {}

    std::string              name;
    uint32_t                 color;
    uint32_t                 radius;
    Resources                resources;
    std::vector<RuleUnit*>   rules; // Shall not be free and pointer stay valid.
    std::vector<std::string> targets;
};

//==========================================================================
//! \brief Type of Map (water, grass ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - map Water color 0x0000FF capacity 100 rules [ ]
//!  - map Grass color 0x00FF00 capacity 10 rules [ CreateGrass ]
//==========================================================================
class MapType
{
public:

    MapType(MapType const&) = default;

    MapType(std::string const& name_)
        : name(name_), color(0xFFFFFF), capacity(Resource::MAX_CAPACITY)
    {}

    MapType(std::string const& name_, uint32_t color_, uint32_t capacity_,
            std::initializer_list<RuleMap*> list = {})
        : name(name_), color(color_), capacity(capacity_), rules(list)
    {}

    std::string           name;
    uint32_t              color;
    uint32_t              capacity;
    std::vector<RuleMap*> rules; // Shall not be free and pointer stay valid.
};

//==============================================================================
//! \brief Type of Agents (people, worker ...).
//! Class constructed during the parsing of simulation scripts. This class is
//! shared as read only by several Agent and therefore used as const reference.
//!
//! Examples:
//!  - agent People color 0xFFFF00 speed 10.5
//!  - agent Worker color 0xFFFFFF speed 10 radius 3
//==============================================================================
class AgentType
{
public:

    //--------------------------------------------------------------------------
    //! \brief Copy constructor.
    //--------------------------------------------------------------------------
    AgentType(AgentType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief Empty constructor with default parameters.
    //--------------------------------------------------------------------------
    AgentType(std::string const& name_)
        : name(name_), speed(1.0f), radius(1u), color(0xFFFF00)
    {}

    //--------------------------------------------------------------------------
    //! \brief Constructor with the full values to set.
    //--------------------------------------------------------------------------
    AgentType(std::string const& name_, float speed_, uint32_t radius_, uint32_t color_)
        : name(name_), speed(speed_), radius(radius_), color(color_)
    {}

    //! \brief Name of the Agent (ie. People, Worker ...)
    std::string name;
    //! \brief Max velocity on Ways of Path.
    float       speed ;
    //! \brief Radius action on Maps.
    uint32_t    radius;
    //! \brief For Rendering agents.
    uint32_t    color;
};

//==========================================================================
//! \brief Type of Ways (people, worker ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - segment Dirt color 0xAAAAAA
//==========================================================================
class WayType
{
public:

    WayType(WayType const&) = default;

    WayType(std::string const& name_)
        : name(name_), color(0xFFFFFF)
    {}

    WayType(std::string const& name_, uint32_t color_)
        : name(name_), color(color_)
    {}

    std::string name;
    uint32_t    color;
};

//==========================================================================
//! \brief Type of Ways (people, worker ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - path Road color 0xAAAAAA
//==========================================================================
class PathType
{
public:

    PathType(PathType const&) = default;

    PathType(std::string const& name_)
        : name(name_), color(0xFFFFFF)
    {}

    PathType(std::string const& name_, uint32_t color_)
        : name(name_), color(color_)
    {}

    std::string name;
    uint32_t    color;
};

#endif
