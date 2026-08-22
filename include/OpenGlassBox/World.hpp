//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_WORLD_HPP
#  define OPEN_GLASSBOX_WORLD_HPP

#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/SimulationClock.hpp"

namespace ogb {

//==============================================================================
//! \brief The shared ground every City is founded on.
//!
//! There is one grid for the whole world and one Map per resource type, held
//! here rather than by each City. Two cities that touch therefore share the
//! pollution and the land value along their border, and can no longer be laid
//! out on grids that overlap while pretending to be independent.
//!
//! A City keeps what makes it a city: a name, its own budget, its roads, its
//! buildings. What it administers on the grid is a region, and that is what
//! bounds the rules run on its behalf.
//==============================================================================
class World
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create an empty world. The config gives the size of a cell and
    //! the settings every City inherits.
    // -------------------------------------------------------------------------
    explicit World(SimulationConfig const& config = {});

    // -------------------------------------------------------------------------
    //! \brief Create the Map holding the given resource type, or return the one
    //! already created for it. Maps are shared: a City asking for a map that
    //! another City already created gets that one.
    // -------------------------------------------------------------------------
    Map& addMap(MapType const& type);

    // -------------------------------------------------------------------------
    //! \brief The Map of the given name.
    //! \throw std::out_of_range if no map of that name exists.
    // -------------------------------------------------------------------------
    Map& getMap(std::string const& name);
    Map const& getMap(std::string const& name) const;

    // -------------------------------------------------------------------------
    //! \brief The Map of the given name, or nullptr.
    // -------------------------------------------------------------------------
    Map* findMap(std::string const& name);

    // -------------------------------------------------------------------------
    //! \brief Found a City administering sizeU x sizeV cells starting at the
    //! given world position. Replaces the City of the same name if any.
    // -------------------------------------------------------------------------
    City& addCity(std::string const& name, Vector3f const& position,
                  uint32_t sizeU, uint32_t sizeV);
    City& addCity(std::string const& name, uint32_t sizeU, uint32_t sizeV);

    // -------------------------------------------------------------------------
    //! \brief The City of the given name.
    //! \throw std::out_of_range if no city of that name exists.
    // -------------------------------------------------------------------------
    City& getCity(std::string const& name);
    City const& getCity(std::string const& name) const;

    // -------------------------------------------------------------------------
    //! \brief Move every City forward by one tick, then run the rules of the
    //! maps. Agents move first so that the Units see where they arrived, and
    //! the maps last so that a cell reflects what happened during the tick.
    // -------------------------------------------------------------------------
    void update(float dt);

    // -------------------------------------------------------------------------
    //! \brief Length of the side of a cell, in world units.
    // -------------------------------------------------------------------------
    float cellSize() const { return m_config.gridCellSize; }

    // -------------------------------------------------------------------------
    //! \brief The cell containing the given world position. Coordinates are
    //! signed and unbounded: any position lands on a cell.
    // -------------------------------------------------------------------------
    void world2mapPosition(Vector3f const& worldPos, int32_t& u,
                           int32_t& v) const;

    // -------------------------------------------------------------------------
    //! \brief Position, in world units, of the top-left corner of the cell.
    // -------------------------------------------------------------------------
    Vector3f mapPosition2world(int32_t u, int32_t v) const;

    // -------------------------------------------------------------------------
    //! \brief The runtime settings shared by every City. They can be tuned
    //! live, except for the cell size which the cities read at creation.
    // -------------------------------------------------------------------------
    SimulationConfig const& config() const { return m_config; }
    SimulationConfig& config() { return m_config; }

    SimulationClock const& clock() const { return m_clock; }
    SimulationClock& clock() { return m_clock; }

    Maps& maps() { return m_maps; }
    Maps const& maps() const { return m_maps; }
    Cities& cities() { return m_cities; }
    Cities const& cities() const { return m_cities; }

private:

    //! \brief Settings shared by the whole world.
    SimulationConfig m_config;
    //! \brief Calendar of the simulation, advanced once per tick.
    SimulationClock  m_clock;
    //! \brief One Map per resource type, shared by every City. Declared before
    //! the cities so that it outlives the Units still pointing at it.
    Maps             m_maps;
    //! \brief The regions the world is divided into.
    Cities           m_cities;
};

} // namespace ogb

#endif
