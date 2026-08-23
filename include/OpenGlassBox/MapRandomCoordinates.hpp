//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file MapRandomCoordinates.hpp
//! \brief Walk the cells of a Map once each, in no particular order.

#ifndef OPEN_GLASSBOX_MAPRANDOMCOORDINATES_HPP
#define OPEN_GLASSBOX_MAPRANDOMCOORDINATES_HPP

#include <cstdint>
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief Hands out the cells of a rectangle one at a time, each exactly once,
//! in a shuffled order.
//!
//! A map rule that only acts on part of its cells has to pick which ones, and
//! picking them in reading order would draw a front sweeping the map from one
//! corner. Grass would grow in stripes. What the rule wants is a sample spread
//! over the whole map, which is what walking the cells shuffled gives, and the
//! shuffle has to be a permutation: a rule that fires on nine cells out of ten
//! must not visit the same cell twice and leave another one dry for ever.
//!
//! The permutation is drawn lazily, one cell per call, by swapping the picked
//! cell with the last of the candidates. The cells handed out are kept aside
//! and put back by init(), so the vectors are allocated once for the life of
//! the Map: a large city runs this for every cell of every map at every tick.
//!
//! Coordinates are packed as two sixteen bit halves of one integer, which is
//! what makes the candidate list a flat vector of integers rather than a vector
//! of pairs. A Map is therefore limited to 65536 cells on a side.
//!
//! Example:
//! \code
//! MapRandomCoordinates walk;
//! walk.init(sizeU, sizeV);          // shuffle the whole map
//!
//! uint32_t u, v;
//! uint32_t const wanted = rule.percent(sizeU * sizeV);
//! for (uint32_t i = 0u; (i < wanted) && walk.next(u, v); ++i)
//! {
//!     // apply the rule on the cell (u, v)
//! }
//! \endcode
//!
//! The matching script, where the percentage is what makes only part of the map
//! move on each run:
//! \code
//! mapRule CreateGrass
//!     rate 20 minutes
//!     map Water remove 10 randomTilesPercent 90
//!     map Grass add 1
//! end
//! \endcode
//==============================================================================
class MapRandomCoordinates
{
public:

    MapRandomCoordinates() = default;

    //--------------------------------------------------------------------------
    //! \brief Start a fresh walk over a rectangle of that size.
    //!
    //! Called at the start of every run of a map rule. The cells handed out by
    //! the previous walk are taken back, and nothing is allocated as long as
    //! the size has not changed. A walk left unfinished is finished here, so
    //! that every cell is a candidate again.
    //!
    //! \param[in] mapSizeU number of columns.
    //! \param[in] mapSizeV number of rows.
    //--------------------------------------------------------------------------
    void init(uint32_t mapSizeU, uint32_t mapSizeV);

    // -------------------------------------------------------------------------
    //! \brief Hand out the next cell of the walk.
    //! \param[out] u column of the cell, in [0..mapSizeU[. Zero when there is
    //! nothing left.
    //! \param[out] v row of the cell, in [0..mapSizeV[. Zero when there is
    //! nothing left.
    //! \return true when a cell was handed out, false once every cell of the
    //! rectangle has been visited.
    // -------------------------------------------------------------------------
    bool next(uint32_t& u, uint32_t& v);

private:

    //! \brief Cells not handed out yet, packed as (u << 16) | v.
    std::vector<uint32_t> m_randomCoordinates;
    //! \brief Cells handed out by the current walk, put back by init().
    std::vector<uint32_t> m_returnedCoordinates;
};

} // namespace ogb

#endif
