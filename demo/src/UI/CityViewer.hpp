//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file CityViewer.hpp
//! \brief Interactive city canvas: rendering, picking, zoom and pan.

#ifndef OPEN_GLASSBOX_DEMO_CITY_VIEWER_HPP
#define OPEN_GLASSBOX_DEMO_CITY_VIEWER_HPP

#include "Game/DebugState.hpp"
#include "Host/OpenGL.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/Vector.hpp"

namespace ogb
{
namespace editor
{
class Editor;
}

namespace ui
{

// ****************************************************************************
//! \brief The city view: an ImGui child window whose content is drawn with an
//! ImDrawList. Everything is vectorial, so there is no bitmap font and no
//! texture to ship, and the whole view scales with the zoom.
// ****************************************************************************
class CityViewer
{
public:

    // ------------------------------------------------------------------------
    //! \brief Draw the panel. Must be called between ImGui::NewFrame and
    //! ImGui::Render.
    //! \param[in] simulation: the simulation to display.
    //! \param[in,out] state: what to draw and what got selected.
    //! \param[in,out] editor: the armed tool, which gets first refusal on the
    //! mouse.
    // ------------------------------------------------------------------------
    void draw(Simulation& simulation,
              game::DebugState& state,
              editor::Editor& editor);

    // ------------------------------------------------------------------------
    //! \brief Frame the whole simulation in the view. Called on startup and by
    //! the recenter button.
    // ------------------------------------------------------------------------
    void frameAll(Simulation& simulation);

    // ------------------------------------------------------------------------
    //! \brief Ask for a reframing at the next draws, once the size of the
    //! canvas is known.
    // ------------------------------------------------------------------------
    void requestFrameAll()
    {
        m_frames_to_reframe = REFRAME_FRAMES;
    }

    float zoom() const
    {
        return m_zoom;
    }

    // ------------------------------------------------------------------------
    //! \brief Convert a position of the simulation into a pixel of the canvas.
    // ------------------------------------------------------------------------
    ImVec2 worldToScreen(Vector3f const& world) const;
    ImVec2 worldToScreen(float x, float y) const;

    // ------------------------------------------------------------------------
    //! \brief Convert a pixel of the canvas into a position of the simulation.
    // ------------------------------------------------------------------------
    ImVec2 screenToWorld(ImVec2 const& screen) const;

    // ------------------------------------------------------------------------
    //! \brief World position of the mouse, which the editing tools work in.
    // ------------------------------------------------------------------------
    ImVec2 mouseWorld() const
    {
        return screenToWorld(ImGui::GetIO().MousePos);
    }

    // ------------------------------------------------------------------------
    //! \brief Segment of the given city closest to a world position, within a
    //! tolerance expressed in pixels so that it stays usable at any zoom.
    //! \param[out] offset: where the projection lands on the segment, in [0..1]
    //! from its origin node.
    //! \return the segment, or nullptr when nothing is close enough.
    // ------------------------------------------------------------------------
    Segment*
    pickSegment(City& city, ImVec2 const& world, float pixels, float& offset) const;

    // ------------------------------------------------------------------------
    //! \brief Node of the given city closest to a world position, within a
    //! tolerance in pixels.
    // ------------------------------------------------------------------------
    Node* pickNode(City& city, ImVec2 const& world, float pixels) const;

    // ------------------------------------------------------------------------
    //! \brief Unit of the given city closest to a world position, within a
    //! tolerance in pixels.
    // ------------------------------------------------------------------------
    Unit* pickUnit(City& city, ImVec2 const& world, float pixels) const;

    // ------------------------------------------------------------------------
    //! \brief Agent of the given city closest to a world position, within a
    //! tolerance in pixels.
    // ------------------------------------------------------------------------
    Agent* pickAgent(City& city, ImVec2 const& world, float pixels) const;

private:

    // ------------------------------------------------------------------------
    //! \brief Pan with the middle button, zoom with the wheel centered on the
    //! cursor, and pick the entity under a left click.
    // ------------------------------------------------------------------------
    void handleInputs(Simulation& simulation,
                      game::DebugState& state,
                      editor::Editor& editor);

    // ------------------------------------------------------------------------
    //! \brief Select the entity closest to the given canvas position, within a
    //! tolerance in pixels. Units win over Agents, which win over Nodes, and a
    //! click on nothing falls back on the grid cell.
    // ------------------------------------------------------------------------
    void
    pick(Simulation& simulation, game::DebugState& state, ImVec2 const& screen);

    // ------------------------------------------------------------------------
    //! \brief Entity under the given canvas position, without writing state.
    // ------------------------------------------------------------------------
    game::Selection pickAt(Simulation& simulation,
                           game::DebugState const& state,
                           ImVec2 const& screen) const;

    // ------------------------------------------------------------------------
    //! \brief How many cells on a side one drawn square of a layer covers.
    //!
    //! One when the zoom makes a cell large enough to be worth a rectangle of
    //! its own and there are few enough of them on screen; more when either
    //! would cost too much. See Layer::forEachBlockInRegion.
    //!
    //! \param[in] pixels how wide one cell is on screen.
    //! \param[in] visible the cells the canvas shows.
    //! \return a divisor of Layer::CHUNK_SIZE, at least one.
    // ------------------------------------------------------------------------
    static int32_t cellsPerSquare(float const pixels, CellRegion const& visible);

    void drawLayers(Simulation& simulation, game::DebugState const& state);
    void drawPaths(City& city, game::DebugState const& state);
    void drawUnits(City& city, game::DebugState const& state);
    void drawAgents(City& city, game::DebugState const& state);
    void drawCityFrame(City const& city, game::DebugState const& state);
    void drawZones(City& city);
    void drawSelectionOverlay(Simulation& simulation,
                              game::DebugState const& state,
                              editor::Editor const& editor);
    void drawInspectHover(Simulation& simulation,
                          game::DebugState const& state,
                          editor::Editor const& editor);
    void drawLegend(game::DebugState const& state);
    void drawToolbar(Simulation& simulation,
                     game::DebugState& state,
                     editor::Editor& editor);

    // ------------------------------------------------------------------------
    //! \brief Update state.hoveredCell from the mouse position.
    //! \param[in] hovered: whether the mouse is over the canvas surface. It is
    //! passed in rather than queried again: a window stops reporting itself as
    //! hovered on the very frame an item of it becomes active, which is exactly
    //! the frame the brushes need the cell on.
    // ------------------------------------------------------------------------
    void
    updateHover(Simulation& simulation, game::DebugState& state, bool hovered);

    // ------------------------------------------------------------------------
    //! \brief Tooltip listing the value of every Layer on the hovered cell.
    // ------------------------------------------------------------------------
    void drawHoverTooltip(Simulation& simulation,
                          game::DebugState const& state,
                          editor::Editor const& editor);
    void drawDisplayToggles(game::DebugState& state) const;
    void drawClockHud(Simulation const& simulation);
    void drawHint(editor::Editor const& editor);

private:

    //! \brief Layers of the drawing, submitted out of order and merged in this
    //! order by the ImDrawListSplitter.
    enum Channel
    {
        CHANNEL_MAPS = 0,
        CHANNEL_GRID,
        CHANNEL_WAYS,
        CHANNEL_NODES,
        CHANNEL_UNITS,
        CHANNEL_AGENTS,
        CHANNEL_OVERLAY,
        CHANNEL_COUNT,
    };

    //! \brief Canvas position, in pixels, of the world origin.
    ImVec2 m_offset = ImVec2(0.0f, 0.0f);
    //! \brief Pixels per world unit.
    float m_zoom = 1.0f;
    //! \brief Top-left corner of the canvas, in screen pixels.
    ImVec2 m_canvas_origin = ImVec2(0.0f, 0.0f);
    ImVec2 m_canvas_size = ImVec2(0.0f, 0.0f);
    ImDrawList* m_draw_list = nullptr;
    ImDrawListSplitter m_splitter;

    //! \brief The dockspace resizes the panel over the first frames, so keep
    //! reframing for a few of them rather than settling on a size that is about
    //! to change.
    static constexpr int REFRAME_FRAMES = 4;
    int m_frames_to_reframe = REFRAME_FRAMES;
};
} // namespace ui
} // namespace ogb

#endif
