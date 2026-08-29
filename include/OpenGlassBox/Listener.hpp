//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Listener.hpp
//! \brief Callbacks for world-level events.

#ifndef OPEN_GLASSBOX_LISTENER_HPP
#define OPEN_GLASSBOX_LISTENER_HPP

#include "OpenGlassBox/Vector.hpp"

#include <string>

namespace ogb
{

class City;
class Segment;

//==============================================================================
//! \brief Events for the whole world: cities added or removed, and roads that
//! cross a city border.
//!
//! One interface for all events. The application registers one object with
//! Simulation::setListener(). Events inside one city use City::Listener instead.
//!
//! Every method has a default. Override only what you need.
//! By default, a road may cross a border. This fits a single-city application.
//!
//! Example:
//! \code
//! struct MyListener: ogb::Simulation::Listener
//! {
//!     void onCityAdded(ogb::City& city) override
//!     {
//!         ogb::installDijkstraRouter(city, m_simulation.getConfig());
//!     }
//! };
//! \endcode
//==============================================================================
class SimulationListener
{
public:

    //==========================================================================
    //! \brief A road segment waiting for approval.
    //!
    //! Uses world coordinates and a type name, not pointers.
    //! The neighbour city can read it before the segment exists.
    //==========================================================================
    struct SegmentProposal
    {
        //! \brief Start point, in world coordinates.
        Vector3f from;
        //! \brief End point, in world coordinates.
        Vector3f to;
        //! \brief Name of the segment type, such as "Dirt".
        std::string segmentType;
    };

    virtual ~SimulationListener() = default;

    //--------------------------------------------------------------------------
    //! \brief A city was created.
    //! \param[in] city the new city. Install its router here.
    //--------------------------------------------------------------------------
    virtual void onCityAdded(City& /*city*/) {}

    //--------------------------------------------------------------------------
    //! \brief A city is about to be removed, with everything it holds.
    //! \param[in] city the city. It is still valid during this call.
    //--------------------------------------------------------------------------
    virtual void onCityRemoved(City& /*city*/) {}

    //--------------------------------------------------------------------------
    //! \brief May a road be built inside another city?
    //! \param[in] owner the city that wants to build the road.
    //! \param[in] neighbor the city the segment would stand in.
    //! \param[in] proposal the segment, in world coordinates.
    //! \return false to refuse. The whole road is cancelled. Nothing is built.
    //--------------------------------------------------------------------------
    virtual bool allowSegmentAcross(City& /*owner*/,
                                City& /*neighbor*/,
                                SegmentProposal const& /*proposal*/)
    {
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief May a road inside another city be demolished?
    //! \param[in] owner the city that asks for the removal.
    //! \param[in] neighbor the city that owns the segment.
    //! \param[in] segment the segment to demolish.
    //! \return false to refuse.
    //--------------------------------------------------------------------------
    virtual bool
    allowSegmentRemoved(City& /*owner*/, City& /*neighbor*/, Segment& /*segment*/)
    {
        return true;
    }
};

} // namespace ogb

#endif
