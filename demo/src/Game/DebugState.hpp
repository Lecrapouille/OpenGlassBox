//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file DebugState.hpp
//! \brief Shared debug UI state: selection, layer visibility and display options.


#ifndef OPEN_GLASSBOX_DEMO_DEBUG_STATE_HPP
#  define OPEN_GLASSBOX_DEMO_DEBUG_STATE_HPP

#  include "OpenGlassBox/Simulation.hpp"

#  include <cstdint>
#  include <map>
#  include <string>


namespace ogb {
namespace game {


// ****************************************************************************
//! \brief How a Layer is painted over the grid.
// ****************************************************************************
enum class LayerMode
{
    //! \brief Fill the whole cell with the layer color, the amount of resource
    //! driving the opacity. This is the readable representation, and the one
    //! that replaces the old rendering where the *size* of the square encoded
    //! the ratio, unreadable as soon as two layers overlapped.
    Heatmap,
    //! \brief Outline the cells holding a resource, so that several layers can be
    //! superimposed without hiding each other.
    Contour,
    //! \brief Print the raw amount in each cell. Only legible when zoomed in.
    Value,
};

// ****************************************************************************
//! \brief Per Layer display settings, keyed by layer name so that they survive the
//! reload of a simulation script.
// ****************************************************************************
struct LayerSettings
{
    bool visible = true;
    float opacity = 0.75f;
    LayerMode mode = LayerMode::Heatmap;
};

// ****************************************************************************
//! \brief What the Inspector shows. Units and Nodes are stable addresses, an
//! Agent is referenced by its identifier because it is destroyed as soon as it
//! delivered its resources.
// ****************************************************************************
struct Selection
{
    enum class Kind
    {
        None,
        Unit,
        Node,
        Agent,
        Segment,
        Zone,
        Cell,
    };

    Kind kind = Kind::None;
    //! \brief Name of the City the selected entity belongs to.
    std::string city;
    Unit* unit = nullptr;
    Node* node = nullptr;
    Segment* segment = nullptr;
    Zone* zone = nullptr;
    //! \brief Agent::id() of the selected Agent.
    uint32_t agentId = 0u;
    int32_t u = 0;
    int32_t v = 0;

    void clear() { *this = Selection(); }

    //! \brief Resolve the selected Agent, or nullptr when it has been removed
    //! from the simulation since the selection was made.
    Agent* resolveAgent(Simulation& simulation) const;
};

// ****************************************************************************
//! \brief Everything the panels and the canvas share. Owned by GlassBoxApp and
//! handed by reference, which keeps the panels free of any dependency on each
//! other.
// ****************************************************************************
struct DebugState
{
    //! \brief Display settings of every known Layer, by layer name.
    std::map<std::string, LayerSettings> layers;
    //! \brief Name of the Layer shown as a full cell heatmap, empty for none.
    //! The others are drawn as overlays.
    std::string primaryLayer;
    //! \brief When set, only this Layer is drawn.
    std::string soloLayer;

    bool showGrid = true;
    bool showPaths = true;
    bool showUnits = true;
    bool showAgents = true;
    bool showNodes = true;
    bool showLabels = true;
    bool showZones = true;
    //! \brief Color and thicken the Segments by their flow over capacity ratio.
    bool showTraffic = true;
    //! \brief Draw the layerRadius disc of the selected Unit.
    bool showSelectionRadius = true;

    Selection selection;
    //! \brief Cell under the mouse, refreshed by the canvas every frame.
    bool hasHoveredCell = false;
    int32_t hoveredU = 0;
    int32_t hoveredV = 0;
    std::string hoveredCity;

    //--------------------------------------------------------------------------
    //! \brief Settings of a Layer, created with the defaults on first use.
    //--------------------------------------------------------------------------
    LayerSettings& layer(std::string const& name) { return layers[name]; }

    //--------------------------------------------------------------------------
    //! \brief Whether a Layer shall be drawn, taking the solo mode into account.
    //--------------------------------------------------------------------------
    bool isLayerVisible(std::string const& name) const
    {
        if (!soloLayer.empty())
            return name == soloLayer;

        auto const it = layers.find(name);
        return (it == layers.end()) || it->second.visible;
    }
};
} // namespace game
} // namespace ogb

#endif
