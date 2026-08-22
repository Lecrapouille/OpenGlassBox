//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file MapRandomCoordinates.hpp
//! \brief Shuffled cell coordinates used to iterate maps in random order.


#ifndef OPEN_GLASSBOX_MAPRANDOMCOORDINATES_HPP
#  define OPEN_GLASSBOX_MAPRANDOMCOORDINATES_HPP

#  include <vector>
#  include <stdint.h>

namespace ogb {

//==============================================================================
//! \brief
//==============================================================================
class MapRandomCoordinates
{
public:

    MapRandomCoordinates() = default;

    //--------------------------------------------------------------------------
    //! \brief Initialize internal states with values from parameters.
    //! \param[in] mapSizeU: map dimension along axis-U.
    //! \param[in] mapSizeV: map dimension along axis-V.
    //--------------------------------------------------------------------------
    void init(uint32_t mapSizeU, uint32_t mapSizeV);

    // -------------------------------------------------------------------------
    //! \brief Iterator: return the next Map coordinate for iteration around the
    //! Map coordinate initialized from the method init().
    //! \param[out] u: the next Map coordinate on the U-axis.
    //! \param[out] v: the next Map coordinate on the V-axis.
    //! \return true if we can iterate, else return false when reached the last
    //! iteration.
    // -------------------------------------------------------------------------
    bool next(uint32_t& u, uint32_t& v);

private:

    std::vector<uint32_t> m_randomCoordinates;
    std::vector<uint32_t> m_returnedCoordinates;
};

} // namespace ogb

#endif
