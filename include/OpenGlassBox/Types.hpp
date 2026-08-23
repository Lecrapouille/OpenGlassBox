//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Types.hpp
//! \brief Descriptor structs for the paths, ways, units, maps, areas, agents
//! and rules a simulation script defines.
//!
//! Nothing here holds any state of a running city: these are the read-only
//! recipes the parser produces, shared by every entity of the same kind. A city
//! of four hundred houses holds one UnitType and four hundred Unit, each of
//! them keeping a reference to it, which is why these objects must outlive the
//! City and must not move in memory. ScriptDefinitions owns them.

#ifndef OPEN_GLASSBOX_TYPES_HPP
#define OPEN_GLASSBOX_TYPES_HPP

#include "OpenGlassBox/Resources.hpp"

namespace ogb
{

class RuleUnit;
class RuleMap;
class RuleArea;
class IRuleCommand;

//==============================================================================
//! \brief What every type a script declares has in common: the name it is
//! referred to by, and the colour the demo paints it with.
//!
//! The name is the identity of the type: rules, units and maps refer to each
//! other by name, and a save file names what it puts back on the map. The
//! colour belongs here rather than in the renderer because a script author
//! choosing what a thing is also chooses what it looks like.
//==============================================================================
struct EntityType
{
    //! \brief Name given by the script, unique among the types of its kind.
    std::string name;
    //! \brief 0xRRGGBB, as written in the script.
    uint32_t color = 0xFFFFFF;
};

//==============================================================================
//! \brief What every rule a script declares has in common: a name, how often it
//! is attempted, and the list of commands making up its body.
//!
//! A rule is a transaction: every command validates, or nothing is applied. See
//! IRule::execute.
//==============================================================================
struct RuleType
{
    //! \brief Name given by the script, by which units, maps and areas list the
    //! rules they run.
    std::string name;
    //! \brief Period in simulation ticks, as written by \c rate \c 7. One means
    //! every tick. Meaningless when \c rateMinutes is set.
    uint32_t rate = 1u;
    //! \brief Period written as a duration of game time, in minutes, as written
    //! by \c rate \c 30 \c minutes. Zero when the script counted ticks instead.
    //! Turned into ticks by IRule::periodTicks, which is what lets the whole
    //! ruleset follow SimulationConfig::ticksPerMinute.
    uint32_t rateMinutes = 0u;
    //! \brief Body of the rule, in the order the script wrote it. The pointers
    //! are owned by ScriptDefinitions: they are never freed here and have to
    //! stay valid for as long as the rule may run.
    std::vector<IRuleCommand*> commands;
};

//==============================================================================
//! \brief Recipe of a rule run by a Map, on one cell of the grid at a time.
//!
//! A map rule may run on a random sample of its cells rather than on all of
//! them, which is how a slow diffusion is written without giving every cell its
//! own rule.
//!
//! Example:
//! \code
//! mapRule CreateGrass
//!     rate 20 minutes
//!     randomTilesPercent 90
//!     map Water remove 10
//!     map Grass add 1
//! end
//! \endcode
//==============================================================================
class RuleMapType: public RuleType
{
public:

    RuleMapType(RuleMapType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the rule.
    //--------------------------------------------------------------------------
    explicit RuleMapType(std::string const& name_)
    {
        name = name_;
    }

    //! \brief Whether the rule runs on a sample of the cells of the Map instead
    //! of on every one of them.
    bool randomTiles = false;

    //! \brief Size of that sample, in percent of the cells of the Map. Clamped
    //! to a hundred by RuleMap.
    uint32_t randomTilesPercent = 10u;
};

//==============================================================================
//! \brief Recipe of a rule run by a Unit, once every \c rate on the building
//! itself.
//!
//! A unit rule is where the traffic of a city comes from: it turns resources
//! into other resources, and sends Agents to look for a building that accepts
//! what it produced.
//!
//! Example:
//! \code
//! unitRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 18
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class RuleUnitType: public RuleType
{
public:

    RuleUnitType(RuleUnitType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the rule.
    //--------------------------------------------------------------------------
    explicit RuleUnitType(std::string const& name_)
    {
        name = name_;
    }

    //! \brief Rule run instead when this one does not fire, or nullptr. Owned
    //! by ScriptDefinitions. This is how a script writes an alternative: try to
    //! sell, and failing that, throw the stock away.
    RuleUnit* onFail = nullptr;
};

//==============================================================================
//! \brief Recipe of a rule run by an Area, on the zone as a whole.
//!
//! Area rules are the only ones that create and destroy buildings, which is why
//! they count what already stands inside the zone.
//!
//! Example:
//! \code
//! areaRule GrowHomes
//!     rate 4 hours
//!     count Home less 12
//!     spawn Home at nearestWay
//! end
//! \endcode
//==============================================================================
class RuleAreaType: public RuleType
{
public:

    RuleAreaType(RuleAreaType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the rule.
    //--------------------------------------------------------------------------
    explicit RuleAreaType(std::string const& name_)
    {
        name = name_;
    }
};

//==============================================================================
//! \brief Recipe of a building: what it holds, what it does and what may be
//! delivered to it.
//!
//! Example:
//! \code
//! unit Home color 0xFF00FF mapRadius 1 rules [ SendPeopleToWork ]
//!      targets [ Home ] caps [ People 8 ] resources [ People 8 ]
//! \endcode
//!
//! \code
//! // The type is looked up by name and outlives every building of that type.
//! UnitType const& type = simulation.script().getUnitType("Home");
//! Unit& home = city.addUnit(type, node);
//! \endcode
//==============================================================================
class UnitType: public EntityType
{
public:

    UnitType(UnitType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the building, and the name
    //! Agents look for. Every other field takes its default.
    //--------------------------------------------------------------------------
    explicit UnitType(std::string const& name_)
    {
        name = name_;
    }

    //! \brief How far, in grid cells, the rules of the building read and write
    //! the Maps around it. One means the cell it stands on and its neighbours.
    uint32_t radius = 1u;

    //! \brief What a fresh building of that type holds, and how much of each
    //! resource it can ever hold. The amounts are the starting stock; the
    //! capacities are the ceiling its rules and its deliveries respect.
    Resources resources;

    //! \brief Rules the building attempts, in the order the script listed them.
    //! Owned by ScriptDefinitions; may contain nullptr when the script named a
    //! rule that does not exist.
    std::vector<RuleUnit*> rules;

    //! \brief The names an Agent may look for to end its trip here. A building
    //! answering to no name at all can never be delivered to.
    std::vector<std::string> targets;
};

//==============================================================================
//! \brief Recipe of a heatmap: one number per cell of the grid, and the rules
//! that make it move.
//!
//! Example:
//! \code
//! map Water color 0x0000FF capacity 100 rules [ ]
//! map Grass color 0x00FF00 capacity 10 rules [ CreateGrass ]
//! \endcode
//==============================================================================
class MapType: public EntityType
{
public:

    MapType(MapType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the map. The capacity of a
    //! cell defaults to the largest one a Resource allows.
    //--------------------------------------------------------------------------
    explicit MapType(std::string const& name_)
    {
        name = name_;
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the map.
    //! \param[in] color_ 0xRRGGBB the demo shades the cells with.
    //! \param[in] capacity_ largest amount one cell may hold.
    //! \param[in] list rules the map runs. Not owned.
    //--------------------------------------------------------------------------
    MapType(std::string const& name_,
            uint32_t color_,
            uint32_t capacity_,
            std::initializer_list<RuleMap*> list = {})
        : capacity(capacity_), rules(list)
    {
        name = name_;
        color = color_;
    }

    //! \brief Largest amount one cell may hold. The same for every cell: a Map
    //! is a grid of identical bins.
    uint32_t capacity = Resource::MAX_CAPACITY;

    //! \brief Rules the map attempts. Owned by ScriptDefinitions.
    std::vector<RuleMap*> rules;
};

//==============================================================================
//! \brief Recipe of a traveller: how fast it drives and what it looks like.
//!
//! Shared as read only by every Agent of that type, and held by them as a const
//! reference: a city may have a thousand of them alive at once, and none of
//! them owns anything.
//!
//! Example:
//! \code
//! agent People color 0xFFFF00 speed 10
//! agent Truck color 0x8888FF speed 8
//! \endcode
//==============================================================================
class AgentType: public EntityType
{
public:

    AgentType(AgentType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the agent. Speed, radius
    //! and colour take their defaults.
    //--------------------------------------------------------------------------
    explicit AgentType(std::string const& name_)
    {
        name = name_;
        color = 0xFFFF00;
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the agent.
    //! \param[in] speed_ top speed, in world units per second of game time.
    //! \param[in] radius_ radius of action on the Maps, in grid cells.
    //! \param[in] color_ 0xRRGGBB the demo draws it with.
    //--------------------------------------------------------------------------
    AgentType(std::string const& name_,
              float speed_,
              uint32_t radius_,
              uint32_t color_)
        : speed(speed_), radius(radius_)
    {
        name = name_;
        color = color_;
    }

    //! \brief Top speed, in world units per second of game time. What an Agent
    //! actually drives is the lesser of this and the speed of the Way under it,
    //! so a fast truck on a dirt road is a slow truck.
    float speed = 1.0f;

    //! \brief Radius of action on the Maps, in grid cells. Reserved: an Agent
    //! does not run rules.
    uint32_t radius = 1u;
};

//==============================================================================
//! \brief Recipe of a road segment: how fast it is, how much traffic it takes
//! and how badly it suffers from more.
//!
//! The three numbers are the parameters of the BPR travel time function the
//! router uses. See Way::travelTime and the traffic section of the README.
//!
//! Example:
//! \code
//! segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
//! \endcode
//==============================================================================
class WayType: public EntityType
{
public:

    WayType(WayType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the segment type.
    //--------------------------------------------------------------------------
    explicit WayType(std::string const& name_)
    {
        name = name_;
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the segment type.
    //! \param[in] color_ 0xRRGGBB the demo draws it with, unless the traffic
    //! colours are on.
    //--------------------------------------------------------------------------
    WayType(std::string const& name_, uint32_t color_)
    {
        name = name_;
        color = color_;
    }

    //! \brief Free flow speed, in world units per second of game time. Used to
    //! derive the zero-traffic travel time of a Way from its length.
    float speed = 50.0f;

    //! \brief Number of Agents a Way of this type carries before the travel
    //! time starts to grow noticeably. This is the practical capacity of the
    //! BPR function, not a hard limit: a saturated road stays passable, it just
    //! becomes expensive.
    float capacity = 20.0f;

    //! \brief Exponent of the BPR function. Four is the value published by the
    //! Bureau of Public Roads and the one used by CiudadSim. Raising it makes
    //! congestion bite later and harder.
    float beta = 4.0f;
};

//==============================================================================
//! \brief Recipe of a road network: a family of segments Agents may drive on.
//!
//! Two Paths never meet, which is what separates a road network from a rail
//! network: an Agent routed on one cannot hop onto the other.
//!
//! Example:
//! \code
//! path Road color 0xAAAAAA
//! \endcode
//==============================================================================
class PathType: public EntityType
{
public:

    PathType(PathType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the network.
    //--------------------------------------------------------------------------
    explicit PathType(std::string const& name_)
    {
        name = name_;
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the network.
    //! \param[in] color_ 0xRRGGBB the demo draws its segments with.
    //--------------------------------------------------------------------------
    PathType(std::string const& name_, uint32_t color_)
    {
        name = name_;
        color = color_;
    }
};

//==============================================================================
//! \brief Recipe of a zone: what the player paints on the map, and the rules
//! that grow, upgrade and abandon the buildings inside it.
//!
//! Example:
//! \code
//! area Residential color 0x44AA44 rules [ GrowHomes AbandonHomes ]
//! \endcode
//==============================================================================
class AreaType: public EntityType
{
public:

    AreaType(AreaType const&) = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] name_ name the script gave the zone.
    //--------------------------------------------------------------------------
    explicit AreaType(std::string const& name_)
    {
        name = name_;
        color = 0x44AA44;
    }

    //! \brief Rules the zone attempts. Owned by ScriptDefinitions.
    std::vector<RuleArea*> rules;
};

} // namespace ogb

#endif
