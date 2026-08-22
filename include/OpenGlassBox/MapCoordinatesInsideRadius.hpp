//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file MapCoordinatesInsideRadius.hpp
//! \brief Lookup table of grid cells lying within a circular radius.


#ifndef OPEN_GLASSBOX_MAPCOORDINATESINSIDERADIUS_HPP
#  define OPEN_GLASSBOX_MAPCOORDINATESINSIDERADIUS_HPP

#  include <vector>
#  include <map>
#  include <cstdint>

namespace ogb {

//==============================================================================
//! \brief Utility class computing cell indices of a Map given a position and a
//! radius. This allows an Unit to do action on Maps around a given distance.
//! For example for radius = 1 will get { (-1,0), (0,-1), (0,0), (0,1), (1,0) }.
//==============================================================================
class MapCoordinatesInsideRadius
{
public:

    //--------------------------------------------------------------------------
    //! \brief Storage of relative Map coordinates (u, v) compressed into single
    //! values.
    //--------------------------------------------------------------------------
    using RelativeCoordinates = std::vector<int32_t>;

    //--------------------------------------------------------------------------
    //! \brief Default constructor. Internal states are not initialized. So call
    //! init() to initialize them.
    //--------------------------------------------------------------------------
    MapCoordinatesInsideRadius() = default;

    //--------------------------------------------------------------------------
    //! \brief Initialize internal states with values from parameters.
    //! \param[in] radius: the circle radius in which actions are performed.
    //! \param[in] centerU: the circle center Map coordinate along axis-U.
    //! \param[in] centerU: the circle center Map coordinate along axis-V.
    //! \param[in] minU: minimal coordinate along axis-U clipping coordinates.
    //! \param[in] maxU: maximal coordinate along axis-U clipping coordinates.
    //! \param[in] minV: minimal coordinate along axis-V clipping coordinates.
    //! \param[in] maxV: maximal coordinate along axis-V clipping coordinates.
    //! \param[in] random: if set to true generate random values.
    //--------------------------------------------------------------------------
    void init(uint32_t radius,
              int32_t centerU, int32_t centerV,
              int32_t minU, int32_t maxU,
              int32_t minV, int32_t maxV,
              bool random);

    //--------------------------------------------------------------------------
    //! \brief Iterator: return the next Map coordinate for iteration around the
    //! Map coordinate center and radius set from the method init().
    //! \param[out] u: the next Map coordinate on the U-axis.
    //! \param[out] v: the next Map coordinate on the V-axis.
    //! \return true if we can iterate, else return false when reached the last
    //! iteration.
    //--------------------------------------------------------------------------
    bool next(int32_t& u, int32_t& v);

private:

    //--------------------------------------------------------------------------
    //! \brief Computing relative Map coordinates depending on the given radius
    //! ie. With radius = 1, will cache (-1,0), (0,-1), (0,0), (0,1), (1,0)
    //! \param[in] radius: Radius of actions.
    //! \param[out] coord: return the relative coordinates computed.
    //--------------------------------------------------------------------------
    void createRelativeCoordinates(int32_t radius, RelativeCoordinates &coord);

    //--------------------------------------------------------------------------
    //! \brief Avoid Computing relative coordinates Use a look-up table where
    //! the radius is the key and relative coordinates the data already computed.
    //--------------------------------------------------------------------------
    static RelativeCoordinates& relativeCoordinates(uint32_t radius)
    {
        static std::map<uint32_t, RelativeCoordinates> coordinates;
        return coordinates[radius];
    }

    //--------------------------------------------------------------------------
    //! \brief Compress two numbers into one number. This function only works
    //! with small values.
    //! This function is used to store coordinate along axis U and V as a single
    //! value.
    //--------------------------------------------------------------------------
    static int32_t compress(int32_t u, int32_t v);

    //--------------------------------------------------------------------------
    //! \brief Decompress one number into a pair of number.
    //! This function is used to unstore U and V coordinate.
    //--------------------------------------------------------------------------
    static void uncompress(int32_t val, int32_t& u, int32_t& v);

private:

    static constexpr int32_t MAX_RADIUS = 255;

    //! \brief Cache relative coordinates to the center and depending on radius.
    //! ie. With radius = 1, we will cache (-1,0), (0,-1), (0,0), (0,1), (1,0)
    RelativeCoordinates* m_relativeCoord = nullptr;
    uint32_t m_startingIndex;
    uint32_t m_offset;
    int32_t m_centerU;
    int32_t m_centerV;
    int32_t m_minU;
    int32_t m_maxU;
    int32_t m_minV;
    int32_t m_maxV;
};

} // namespace ogb

#endif
