//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_MAP_HPP
#  define OPEN_GLASSBOX_MAP_HPP

#  include "Core/Unique.hpp"
#  include "Core/Config.hpp"
#  include "Core/Rule.hpp"
#  include "Core/MapCoordinatesInsideRadius.hpp"
#  include "Core/Vector.hpp"

class City;

//==============================================================================
//! \brief Maps are simple uniform size grids. A Map represents a single type of
//! resource in the environment (coal, oil, forest but also air pollution, land
//! value, desirability ...). Each cell of a Map is a Resource. Units interact
//! with maps through their footprint. Resources are limited.
//==============================================================================
class Map
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create a sizeU x sizeV grid where each cell has no resource but
    //! where the map capacity and rules are defined by the type of map.
    // -------------------------------------------------------------------------
    Map(MapType const& type, City& city);
    VIRTUAL ~Map() = default;

    // -------------------------------------------------------------------------
    //! \brief Change the amount of resource at the cell index U,V.
    // -------------------------------------------------------------------------
    void setResource(uint32_t const u, uint32_t const v, uint32_t val);

    // -------------------------------------------------------------------------
    //! \brief Get the amount of resource at the cell index U,V.
    // -------------------------------------------------------------------------
    uint32_t getResource(uint32_t const u, uint32_t const v);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getResource(uint32_t const u, uint32_t const v, uint32_t radius);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getCapacity() const { return m_type.capacity; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void addResource(uint32_t const u, uint32_t const v, uint32_t toAdd);

    // -------------------------------------------------------------------------
    //! \brief Distribute the amount resource toAdd to Map cells inside a circle.
    //!
    //! \param[in] distributed: if set to true cells are randomized and each
    //! distribution makes toAdd reduced. Therefore maybe not all cells are feed.
    //! If set to false, each cell gets the same amount of resource.
    //! \note Amount of resource for each cell are constrained by the gobal capacity
    //! of the map.
    // -------------------------------------------------------------------------
    void addResource(uint32_t const u, uint32_t const v, uint32_t const radius,
                     uint32_t const toAdd, bool distributed = true);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResource(uint32_t const u, uint32_t const v, uint32_t toRemove);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResource(uint32_t const u, uint32_t const v, uint32_t radius,
                        uint32_t toRemove, bool distributed = true);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Vector3f getWorldPosition(uint32_t const u, uint32_t const v);

    // -------------------------------------------------------------------------
    //! \brief Change the position of the Map in the world.
    //! This also change the position of Path, Unit, Agent ... hold by the City.
    // -------------------------------------------------------------------------
    void translate(Vector3f const direction);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    VIRTUAL void executeRules();

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Map.
    // -------------------------------------------------------------------------
    std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief Return the world position of the city (top-left corner).
    // -------------------------------------------------------------------------
    Vector3f const& position() const { return m_position; }

    // -------------------------------------------------------------------------
    //! \brief Return the number of graduations along the U-axis
    // -------------------------------------------------------------------------
    uint32_t gridSizeU() const { return m_gridSizeU; }

    // -------------------------------------------------------------------------
    //! \brief Return the number of graduations along the V-axis
    // -------------------------------------------------------------------------
    uint32_t gridSizeV() const { return m_gridSizeV; }

    // -------------------------------------------------------------------------
    //! \brief Return the color for the renderer.
    // -------------------------------------------------------------------------
    uint32_t const& color() const { return m_type.color; }

private:

    MapType          const& m_type;
    //! \brief Position of the top-left corner.
    Vector3f                m_position;
    //! \brief The size of the grid along the U-axis.
    uint32_t                m_gridSizeU;
    //! \brief The size of the grid along the V-axis.
    uint32_t                m_gridSizeV;
    //! \brief Structure holding all information needed to execute simulation
    //! rules.
    RuleContext             m_context;
    //! \brief Frenquency for running rules.
    uint32_t                m_ticks = 0u;
    //! \brief Amount of resource for each cell of the grid. The capacity is
    //! stired inside MapType.
    std::vector<uint32_t>   m_resources;
    //! \brief Cache coordinates within a position and radius.
    MapCoordinatesInsideRadius m_coordinates;
    // TODO
    //RandomCoordinates       m_randomCoordinate;
};

using Maps = std::map<std::string, std::unique_ptr<Map>>;

#endif
