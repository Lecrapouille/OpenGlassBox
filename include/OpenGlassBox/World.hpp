//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file World.hpp
//! \brief Shared world grid: maps, cities and coordinate conversions.

#ifndef OPEN_GLASSBOX_WORLD_HPP
#define OPEN_GLASSBOX_WORLD_HPP

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/SimulationClock.hpp"

namespace ogb
{

//==============================================================================
//! \brief A road about to be laid, as it is offered to a neighbouring town for
//! approval. Described in world coordinates and by the name of a segment type,
//! rather than by references, so that it means something to a town that has not
//! agreed to it yet.
//==============================================================================
struct WayProposal
{
    //! \brief Where the road starts, in world coordinates.
    Vector3f from;

    //! \brief Where it ends, in world coordinates.
    Vector3f to;

    //! \brief Name of the kind of segment to lay, such as "Dirt".
    std::string wayType;
};

//==============================================================================
//! \brief The ground every town is founded on: one grid, one calendar, and one
//! layer per kind of resource.
//!
//! There is a single grid for the whole world, and the layers of the
//! environment are held here rather than by each town. Two towns that touch
//! therefore share the pollution and the land value along their border, and
//! cannot be laid out on overlapping grids while pretending to be independent.
//!
//! A town keeps what makes it a town: a name, its own budget, its roads, its
//! buildings. What it holds on the grid is the region it administers, and that
//! is what bounds the rules run on its behalf.
//!
//! The World also owns the order of a tick: the towns move their agents and run
//! their buildings first, then the layers run their own rules. Agents move
//! before buildings look around so that a building sees who has just arrived,
//! and the layers come last so that a cell reflects everything that happened
//! during the tick.
//!
//! Example:
//! \code
//! World world;
//! City& paris = world.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f), 32u, 32u);
//! City& orly = world.addCity("Orly", Vector3f(320.0f, 0.0f, 0.0f), 32u, 32u);
//!
//! // One road across the border. It becomes two, one owned by each town, and
//! // Orly gets a say through the listener.
//! world.addRoad(paris, "Road", dirtType,
//!               Vector3f(310.0f, 0.0f, 0.0f), Vector3f(330.0f, 0.0f, 0.0f));
//!
//! // The layer of pollution is the same one for both towns.
//! assert(&paris.getMap("Pollution") == &orly.getMap("Pollution"));
//! \endcode
//==============================================================================
class World
{
public:

    //--------------------------------------------------------------------------
    //! \brief Diplomacy for roads crossing a border between two towns.
    //!
    //! Laying a road across a border creates a piece of it inside a town that
    //! did not ask for it, so the neighbour is given a veto. The default
    //! implementation says yes to everything, which is what a single town demo
    //! wants; a host running several towns, or several processes, can refuse.
    //--------------------------------------------------------------------------
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //--------------------------------------------------------------------------
        //! \brief May a road be laid inside a neighbour?
        //! \param[in] owner the town laying the road.
        //! \param[in] neighbor the town the piece would end up in.
        //! \param[in] proposal the piece, in world coordinates.
        //! \return false to refuse, which makes the whole road fail rather than
        //! leaving half of it laid.
        //--------------------------------------------------------------------------
        virtual bool allowWayAcross(City& /*owner*/,
                                    City& /*neighbor*/,
                                    WayProposal const& /*proposal*/)
        {
            return true;
        }

        //--------------------------------------------------------------------------
        //! \brief May a road inside a neighbour be demolished?
        //! \param[in] owner the town asking.
        //! \param[in] neighbor the town owning the segment.
        //! \param[in] way the segment to demolish.
        //! \return false to refuse.
        //--------------------------------------------------------------------------
        virtual bool
        allowWayRemoved(City& /*owner*/, City& /*neighbor*/, Way& /*way*/)
        {
            return true;
        }
    };

    // -------------------------------------------------------------------------
    //! \brief An empty world: no town, no layer, the clock at the start of the
    //! first day.
    //! \param[in] config the runtime settings, copied. Gives the size of a cell
    //! and everything the towns inherit.
    // -------------------------------------------------------------------------
    explicit World(SimulationConfig const& config = {});

    // -------------------------------------------------------------------------
    //! \brief Install the arbiter of roads crossing borders, replacing the one
    //! saying yes to everything.
    //! \param[in] listener kept by address and has to outlive the World.
    // -------------------------------------------------------------------------
    void setListener(Listener& listener)
    {
        m_listener = &listener;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the arbiter in force, which is the default one when none
    //! was installed.
    // -------------------------------------------------------------------------
    Listener& listener()
    {
        return *m_listener;
    }

    // -------------------------------------------------------------------------
    //! \brief Create the layer for a kind of resource, or hand back the one
    //! already created for it.
    //!
    //! Layers are shared: a town asking for one another town already created
    //! gets that one, which is the whole point of holding them here.
    //!
    //! \param[in] type recipe of the layer, from the ruleset.
    //! \return the layer, owned by the World.
    // -------------------------------------------------------------------------
    Map& addMap(MapType const& type);

    // -------------------------------------------------------------------------
    //! \brief \param[in] name name of the layer, such as "Water".
    //! \return the layer.
    //! \throw std::out_of_range when no layer goes by that name.
    // -------------------------------------------------------------------------
    Map& getMap(std::string const& name);

    //! \copydoc getMap(std::string const&)
    Map const& getMap(std::string const& name) const;

    // -------------------------------------------------------------------------
    //! \brief The same as getMap(), for a caller that would rather test than
    //! catch.
    //! \param[in] name name of the layer.
    //! \return the layer, or nullptr when there is none by that name.
    // -------------------------------------------------------------------------
    Map* findMap(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Found a town administering a rectangle of the grid. A town of the
    //! same name is replaced, with everything it held.
    //! \param[in] name unique name of the town.
    //! \param[in] position world position of the top-left corner of its region.
    //! \param[in] sizeU how many cells it administers along U.
    //! \param[in] sizeV how many cells it administers along V.
    //! \return the new town.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name,
                  Vector3f const& position,
                  uint32_t sizeU,
                  uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief Found a town at the origin of the world. Convenience for a world
    //! holding a single town.
    //! \param[in] name unique name of the town.
    //! \param[in] sizeU how many cells it administers along U.
    //! \param[in] sizeV how many cells it administers along V.
    //! \return the new town.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, uint32_t sizeU, uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief \param[in] name name of the town.
    //! \return the town.
    //! \throw std::out_of_range when no town goes by that name.
    // -------------------------------------------------------------------------
    City& getCity(std::string const& name);

    //! \copydoc getCity(std::string const&)
    City const& getCity(std::string const& name) const;

    // -------------------------------------------------------------------------
    //! \brief One tick of the whole world: every town first, then the layers.
    //!
    //! Agents move before the buildings look around, so that a building sees
    //! who has just arrived, and the layers run last so that a cell reflects
    //! everything that happened during the tick. The calendar is advanced here
    //! too.
    //!
    //! \param[in] dt how long the tick lasts, in seconds of game time.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief \return the side of a cell, in world units. The same for every
    //! layer and every town, which is what makes one grid out of them.
    // -------------------------------------------------------------------------
    float cellSize() const
    {
        return m_config.gridCellSize;
    }

    // -------------------------------------------------------------------------
    //! \brief Which cell of the grid a place falls in.
    //!
    //! Unlike City::world2mapPosition, nothing is clamped: coordinates are
    //! signed and the grid is unbounded, so every place lands on a cell whether
    //! or not a town administers it.
    //!
    //! \param[in] worldPos the place, in world coordinates.
    //! \param[out] u column of the cell.
    //! \param[out] v row of the cell.
    // -------------------------------------------------------------------------
    void
    world2mapPosition(Vector3f const& worldPos, int32_t& u, int32_t& v) const;

    // -------------------------------------------------------------------------
    //! \brief The other way round.
    //! \param[in] u, v coordinates of the cell.
    //! \return the world position of its top-left corner.
    // -------------------------------------------------------------------------
    Vector3f mapPosition2world(int32_t u, int32_t v) const;

    // -------------------------------------------------------------------------
    //! \brief \return the runtime settings shared by every town. They may be
    //! changed while the game runs, apart from the size of a cell, which the
    //! towns read when they are founded.
    // -------------------------------------------------------------------------
    SimulationConfig const& config() const
    {
        return m_config;
    }

    //! \copydoc config() const
    SimulationConfig& config()
    {
        return m_config;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the calendar of the world, advanced once per tick. What
    //! tells a rule whether it is night, and a building whether it is open.
    // -------------------------------------------------------------------------
    SimulationClock const& clock() const
    {
        return m_clock;
    }

    //! \copydoc clock() const
    SimulationClock& clock()
    {
        return m_clock;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the layers of the world, by name.
    // -------------------------------------------------------------------------
    Maps& maps()
    {
        return m_maps;
    }

    //! \copydoc maps()
    Maps const& maps() const
    {
        return m_maps;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the towns of the world, by name.
    // -------------------------------------------------------------------------
    Cities& cities()
    {
        return m_cities;
    }

    //! \copydoc cities()
    Cities const& cities() const
    {
        return m_cities;
    }

    // -------------------------------------------------------------------------
    //! \brief Which town administers a place, which is what the editor asks
    //! before letting the player build there.
    //! \param[in] world the place, in world coordinates.
    //! \return the town whose region holds the cell that place falls in, or
    //! nullptr for open country.
    // -------------------------------------------------------------------------
    City* cityAt(Vector3f const& world);

    //! \copydoc cityAt(Vector3f const&)
    City const* cityAt(Vector3f const& world) const;

    // -------------------------------------------------------------------------
    //! \brief Lay a road from one place to another, cutting it at every border
    //! it crosses.
    //!
    //! The road is clipped against the region of each town, and each piece
    //! belongs to the town it was clipped to: a road across a border becomes
    //! two roads with two owners rather than one road reaching into somebody
    //! else's town. A road falling entirely outside every town is given to the
    //! requester whole. Each neighbour is asked through the Listener before a
    //! piece is created inside it.
    //!
    //! \param[in] owner the town laying the road.
    //! \param[in] pathType name of the network to lay it on, such as "Road". A
    //! town having no network by that name is given one of the same kind as the
    //! requester's; a piece is dropped when the requester has none either.
    //! \param[in] wayType recipe of the segments to lay.
    //! \param[in] from, to the ends of the road, in world coordinates.
    //! \return false when a neighbour refused, in which case nothing at all was
    //! laid: a road is all or nothing.
    // -------------------------------------------------------------------------
    bool addRoad(City& owner,
                 std::string const& pathType,
                 WayType const& wayType,
                 Vector3f const& from,
                 Vector3f const& to);

private:

    //! \brief Settings shared by the whole world.
    SimulationConfig m_config;
    //! \brief Calendar of the world, advanced once per tick.
    SimulationClock m_clock;
    //! \brief One layer per kind of resource, shared by every town. Declared
    //! before the towns so that it outlives the buildings still pointing at it.
    Maps m_maps;
    //! \brief The towns the world is divided into, owned.
    Cities m_cities;
    //! \brief The arbiter used when nobody installed one. Says yes to
    //! everything.
    Listener m_defaultListener;
    //! \brief The arbiter in force. Not owned.
    Listener* m_listener = &m_defaultListener;
};

} // namespace ogb

#endif
