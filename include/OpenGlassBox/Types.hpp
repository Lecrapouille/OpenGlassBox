//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Types.hpp
//! \brief Descriptor structs for script types: Path, Segment, Building, Layer,
//! Zone, Agent and Rule.
//!
//! These hold no City state at runtime.
//! The parser builds them as read-only recipes.
//! Every entity of one kind shares the same type.
//! A City with 400 houses has one BuildingType and 400 Building instances.
//! Each Building keeps a reference to the BuildingType.
//! These objects must outlive the City and must not move in memory.
//! ScriptDefinitions owns them.

#ifndef OPEN_GLASSBOX_TYPES_HPP
#define OPEN_GLASSBOX_TYPES_HPP

#include "OpenGlassBox/Resources.hpp"

namespace ogb
{

class RuleBuilding;
class RuleLayer;
class RuleZone;
class IRuleCommand;

//==============================================================================
//! \brief Common fields for every script type: name and demo colour.
//!
//! The name is the type identity.
//! Rules, Buildings and Layers refer to each other by name.
//! Save files use these names on the grid.
//! Colour is here, not in the renderer.
//! The script author picks both type and look.
//==============================================================================
struct EntityType
{
    //! \brief Name from the script. Unique among types of this kind.
    //! Interned: the engine compares four bytes. See Name.
    //! Still reads and prints as a string.
    Name name;
    //! \brief Colour as 0xRRGGBB, from the script.
    uint32_t color = 0xFFFFFF;
};

//==============================================================================
//! \brief Common fields for every Rule from the script.
//! Name, run period, and command list.
//!
//! A Rule is a transaction: every command validates, or nothing runs.
//! See IRule::execute.
//==============================================================================
struct RuleType
{
    //! \brief Name from the script.
    //! Buildings, Layers and Zones list Rules by this name. Interned. See Name.
    Name name;
    //! \brief Rule body in script order.
    //! ScriptDefinitions owns the command pointers.
    //! They stay valid as long as the Rule may run.
    std::vector<IRuleCommand*> commands;
    //! \brief Period in simulation ticks, from \c rate \c 7.
    //! One means every tick. Not used when \c rateMinutes is set.
    uint32_t rate = 1u;
    //! \brief Period in game minutes, from \c rate \c 30 \c minutes.
    //! Zero when the script counted ticks.
    //! IRule::getPeriodTicks() converts this using TimeConfig::ticksPerMinute.
    uint32_t rateMinutes = 0u;
};

//==============================================================================
//! \brief Recipe for a Layer Rule. It runs on one grid cell at a time.
//!
//! A Layer Rule may run on a random sample of cells, not all cells.
//! This models slow diffusion without one Rule per cell.
//!
//! Example:
//! \code
//! layerRule CreateGrass
//!     rate 20 minutes
//!     randomTilesPercent 90
//!     layer Water remove 10
//!     layer Grass add 1
//! end
//! \endcode
//==============================================================================
class RuleLayerType: public RuleType
{
public:

    RuleLayerType(RuleLayerType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Rule name from the script.
    //--------------------------------------------------------------------------
    explicit RuleLayerType(std::string const& name_)
    {
        name = name_;
    }

    //! \brief Sample size in percent of Layer cells. RuleLayer clamps to 100.
    uint32_t randomTilesPercent = 10u;

    //! \brief Run on a sample of Layer cells instead of every cell. Declared
    //! after the share so that it does not open a gap in front of it.
    bool randomTiles = false;
};

//==============================================================================
//! \brief Recipe for a Building Rule. It runs on the Building every \c rate ticks.
//!
//! Building Rules drive City traffic.
//! They turn resources into other resources.
//! They send Agents to Buildings that accept what they produced.
//!
//! Example:
//! \code
//! buildingRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 18
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class RuleBuildingType: public RuleType
{
public:

    RuleBuildingType(RuleBuildingType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Rule name from the script.
    //--------------------------------------------------------------------------
    explicit RuleBuildingType(std::string const& name_)
    {
        name = name_;
    }

    //! \brief Rule to run when this one fails, or nullptr.
    //! Owned by ScriptDefinitions.
    //! Example: try to sell, else discard stock.
    RuleBuilding* onFail = nullptr;
};

//==============================================================================
//! \brief Recipe for a Zone Rule. It runs on the whole Zone.
//!
//! Only Zone Rules create and destroy Buildings.
//! They count Buildings already in the Zone.
//!
//! Example:
//! \code
//! zoneRule GrowHomes
//!     rate 4 hours
//!     count Home less 12
//!     spawn Home at nearestSegment
//! end
//! \endcode
//==============================================================================
class RuleZoneType: public RuleType
{
public:

    RuleZoneType(RuleZoneType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Rule name from the script.
    //--------------------------------------------------------------------------
    explicit RuleZoneType(std::string const& name_)
    {
        name = name_;
    }
};

//==============================================================================
//! \brief Recipe for a Building: resources, Rules and delivery targets.
//!
//! Example:
//! \code
//! building Home color 0xFF00FF layerRadius 1 rules [ SendPeopleToWork ]
//!      targets [ Home ] caps [ People 8 ] resources [ People 8 ]
//! \endcode
//!
//! \code
//! // Look up by name. The type outlives every Building of that type.
//! BuildingType const& type = simulation.getRuleset().getBuildingType("Home");
//! Building& home = city.addBuilding(type, node);
//! \endcode
//==============================================================================
class BuildingType: public EntityType
{
public:

    BuildingType(BuildingType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Building name from the script.
    //! Also the name Agents look for. Other fields use defaults.
    //--------------------------------------------------------------------------
    explicit BuildingType(std::string const& name_)
    {
        name = name_;
    }

    //! \brief Starting resources and capacities for a new Building of this type.
    //! Amounts are the starting stock.
    //! Capacities limit Rules and deliveries.
    Resources resources;

    //! \brief Rules the Building tries, in script order.
    //! Owned by ScriptDefinitions.
    //! May contain nullptr if the script named a missing Rule.
    std::vector<RuleBuilding*> rules;

    //! \brief Names an Agent may look for to deliver here.
    //! A Building with no targets never receives deliveries.
    //!
    //! Names are interned.
    //! The router checks these thousands of times per tick.
    std::vector<Name> targets;

    //! \brief How far Building Rules read and write Layers, in grid cells.
    //! One means the Building cell and its neighbours. Declared last so that it
    //! sits in the gap the recipe ends on rather than opening one.
    uint32_t radius = 1u;
};

//==============================================================================
//! \brief Recipe for a heatmap Layer: one value per grid cell and its Rules.
//!
//! Example:
//! \code
//! layer Water color 0x0000FF capacity 100 rules [ ]
//! layer Grass color 0x00FF00 capacity 10 rules [ CreateGrass ]
//! \endcode
//!
//! A Layer may also transport and lose its amounts by itself, without a Rule.
//! Smoke moves to the cells nearby and fades in the air:
//! \code
//! layer Pollution color 0x806040 capacity 100 diffusion 24 decay 8 rate 30 minutes rules [ ]
//! \endcode
//==============================================================================
class LayerType: public EntityType
{
public:

    LayerType(LayerType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Layer name from the script.
    //! Cell capacity defaults to the largest Resource allows.
    //--------------------------------------------------------------------------
    explicit LayerType(std::string const& name_)
    {
        name = name_;
    }

    //--------------------------------------------------------------------------
    //! \param[in] name_ Layer name from the script.
    //! \param[in] color_ demo cell colour as 0xRRGGBB.
    //! \param[in] capacity_ maximum amount one cell may hold.
    //! \param[in] list Rules the Layer runs. Not owned.
    //--------------------------------------------------------------------------
    LayerType(std::string const& name_,
            uint32_t color_,
            uint32_t capacity_,
            std::initializer_list<RuleLayer*> list = {})
        : rules(list), capacity(capacity_)
    {
        name = name_;
        color = color_;
    }

    //! \brief Rules the Layer tries. Owned by ScriptDefinitions.
    std::vector<RuleLayer*> rules;

    //! \brief Maximum amount one cell may hold. Same for every cell.
    uint32_t capacity = Resource::MAX_CAPACITY;

    //! \brief Percent of a cell that moves to its four neighbours each period.
    //!
    //! Smoke, noise and fear of crime travel: a factory fouls the street next to
    //! it, not only its own cell. A Rule cannot express this, because a Layer
    //! Rule reads and writes the single cell it stands on, so the engine does it.
    //! Zero, the default, keeps every amount where a Rule put it.
    //!
    //! The four neighbours share the amount equally. The remainder of that
    //! division stays in the cell, so nothing is created and nothing is lost.
    //! Use \c decay for a loss.
    uint32_t diffusion = 0u;

    //! \brief Percent of a cell that disappears each period.
    //!
    //! Pollution settles and noise stops. Without a loss every source fills the
    //! grid until every cell reaches its capacity, and the Layer says nothing
    //! any more. Zero, the default, keeps the amount for ever.
    uint32_t decay = 0u;

    //! \brief Period of \c diffusion and \c decay in ticks, from \c rate \c 7.
    //! One means every tick. Not used when \c rateMinutes is set.
    uint32_t rate = 1u;

    //! \brief Period of \c diffusion and \c decay in game minutes, from
    //! \c rate \c 30 \c minutes. Zero when the script counted ticks.
    uint32_t rateMinutes = 0u;

    //--------------------------------------------------------------------------
    //! \brief Ticks between two transport passes.
    //! Same conversion as IRule::getPeriodTicks().
    //!
    //! \param[in] ticksPerMinute ticks per game minute, from settings.
    //! Zero is treated as one.
    //! \return the period. Never zero.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getPeriodTicks(uint32_t ticksPerMinute) const
    {
        if (rateMinutes == 0u)
            return (rate == 0u) ? 1u : rate;

        uint32_t const perMinute = (ticksPerMinute == 0u) ? 1u : ticksPerMinute;
        return rateMinutes * perMinute;
    }

    //--------------------------------------------------------------------------
    //! \return whether the Layer transports or loses its amounts by itself.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool spreads() const
    {
        return (diffusion != 0u) || (decay != 0u);
    }
};

//==============================================================================
//! \brief Recipe for an Agent: speed and demo look.
//!
//! Shared read-only by every Agent of this type.
//! Each Agent holds a const reference.
//! A City may have thousands of Agents. None owns the type.
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
    //! \param[in] name_ Agent name from the script.
    //! Speed, radius and colour use defaults.
    //--------------------------------------------------------------------------
    explicit AgentType(std::string const& name_)
    {
        name = name_;
        color = 0xFFFF00;
    }

    //--------------------------------------------------------------------------
    //! \param[in] name_ Agent name from the script.
    //! \param[in] speed_ top speed in world units per game second.
    //! \param[in] radius_ Layer action radius in grid cells.
    //! \param[in] color_ demo colour as 0xRRGGBB.
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

    //! \brief Top speed in world units per game second.
    //! An Agent drives at the minimum of this and the Segment speed under it.
    //! A fast truck on a dirt road is slow.
    float speed = 1.0f;

    //! \brief Layer action radius in grid cells. Reserved: Agents do not run Rules.
    uint32_t radius = 1u;
};

//==============================================================================
//! \brief Recipe for a road Segment: speed, capacity and congestion factor.
//!
//! These three values feed the BPR travel time function in the router.
//! See Segment::getTravelTime() and doc/traffic.md.
//!
//! Example:
//! \code
//! segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
//! \endcode
//==============================================================================
class SegmentType: public EntityType
{
public:

    SegmentType(SegmentType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Segment type name from the script.
    //--------------------------------------------------------------------------
    explicit SegmentType(std::string const& name_)
    {
        name = name_;
    }

    //--------------------------------------------------------------------------
    //! \param[in] name_ Segment type name from the script.
    //! \param[in] color_ demo colour as 0xRRGGBB, unless traffic colours are on.
    //--------------------------------------------------------------------------
    SegmentType(std::string const& name_, uint32_t color_)
    {
        name = name_;
        color = color_;
    }

    //! \brief Free-flow speed in world units per game second.
    //! Used with Segment length to get zero-traffic travel time.
    float speed = 50.0f;

    //! \brief Agent count before travel time grows clearly.
    //! Practical BPR capacity, not a hard limit.
    //! A full road stays passable but costs more time.
    float capacity = 20.0f;

    //! \brief BPR exponent. Four is the Bureau of Public Roads value.
    //! Also used by CiudadSim. Higher values mean congestion hits later and harder.
    float beta = 4.0f;
};

//==============================================================================
//! \brief Recipe for a road network: Segments Agents may drive on.
//!
//! Two Paths never meet.
//! This separates road and rail: an Agent on one Path cannot switch to another.
//!
//! Example:
//! \code
//! path Road color 0xAAAAAA
//! path Water color 0x0000FF crossings false
//! \endcode
//==============================================================================
class PathType: public EntityType
{
public:

    PathType(PathType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Path name from the script.
    //--------------------------------------------------------------------------
    explicit PathType(std::string const& name_)
    {
        name = name_;
    }

    //--------------------------------------------------------------------------
    //! \param[in] name_ Path name from the script.
    //! \param[in] color_ demo Segment colour as 0xRRGGBB.
    //--------------------------------------------------------------------------
    PathType(std::string const& name_, uint32_t color_)
    {
        name = name_;
        color = color_;
    }

    //! \brief Whether two Segments that cross make a junction Agents can turn
    //! at. See Path::findCrossings().
    //!
    //! True for a road network: two streets drawn over one another are a
    //! crossroads, and a driver expects to be able to turn there. False for the
    //! networks where one line passing over another means nothing, such as a
    //! water main under a power line.
    bool crossings = true;
};

//==============================================================================
//! \brief Recipe for a Zone: painted area and Rules for Buildings inside it.
//!
//! Example:
//! \code
//! zone Residential color 0x44AA44 rules [ GrowHomes AbandonHomes ]
//! \endcode
//==============================================================================
class ZoneType: public EntityType
{
public:

    ZoneType(ZoneType const&) = default;

    //--------------------------------------------------------------------------
    //! \param[in] name_ Zone name from the script.
    //--------------------------------------------------------------------------
    explicit ZoneType(std::string const& name_)
    {
        name = name_;
        color = 0x44AA44;
    }

    //! \brief Rules the Zone tries. Owned by ScriptDefinitions.
    std::vector<RuleZone*> rules;
};

} // namespace ogb

#endif
