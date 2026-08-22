//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Editor.hpp
//! \brief Interactive map editor: tools, previews and command history.


#ifndef OPEN_GLASSBOX_DEMO_EDITOR_HPP
#  define OPEN_GLASSBOX_DEMO_EDITOR_HPP

#  include "Host/OpenGL.hpp"
#  include "Editor/EditCommands.hpp"
#  include "Game/DebugState.hpp"
#  include "OpenGlassBox/Simulation.hpp"

namespace ogb {
namespace ui { class CityViewer; }

namespace editor {

// ============================================================================
//! \brief What a click on the map does.
// ============================================================================
enum class EditTool
{
    //! \brief Inspect: click to select, which changes nothing in the world.
    Select,
    //! \brief Drag to lay a stretch of road between two points.
    Road,
    //! \brief Click on a road to drop a building on it.
    Building,
    //! \brief Drag a rectangle to paint an Area (a zone).
    Zone,
    //! \brief Drag a rectangle to set an amount of resource on Map cells.
    Paint,
    //! \brief Click to demolish a building or a stretch of road.
    Bulldozer,
};

// ============================================================================
//! \brief The map editor: the current tool, its settings, and the undo history.
//!
//! Every change the editor makes goes through a command, so undo is a property
//! of the editor rather than something each tool reimplements. The tools do the
//! picking and the preview; the commands do the mutation.
// ============================================================================
class Editor
{
public:

    // ------------------------------------------------------------------------
    //! \brief Row of tool buttons and their settings, drawn in the toolbar of
    //! the map panel.
    // ------------------------------------------------------------------------
    void drawToolbar(Simulation& simulation, game::DebugState& state);

    // ------------------------------------------------------------------------
    //! \brief The undo history, as a dockable panel.
    // ------------------------------------------------------------------------
    void drawHistoryPanel(Simulation& simulation);

    // ------------------------------------------------------------------------
    //! \brief Consume the mouse over the canvas. Returns true when the tool
    //! took the click, so the viewer knows not to also treat it as a selection.
    //! \param[in] hovered: whether the mouse is over the canvas.
    // ------------------------------------------------------------------------
    bool onCanvas(Simulation& simulation, game::DebugState& state, ui::CityViewer& viewer,
                  bool hovered);

    // ------------------------------------------------------------------------
    //! \brief Draw what the current tool is about to do: the rubber band of a
    //! road, the rectangle of the brush, the target of the bulldozer.
    // ------------------------------------------------------------------------
    void drawPreview(Simulation& simulation, ui::CityViewer& viewer,
                     ImDrawList* drawList);

    void undo(Simulation& simulation) { m_stack.undo(simulation); }
    void redo(Simulation& simulation) { m_stack.redo(simulation); }
    CommandStack& stack() { return m_stack; }

    // ------------------------------------------------------------------------
    //! \brief Whether a tool other than the selector is armed. The viewer uses
    //! this to change the cursor feedback and to stop picking entities.
    // ------------------------------------------------------------------------
    bool armed() const { return m_tool != EditTool::Select; }

    EditTool tool() const { return m_tool; }
    void setTool(EditTool tool);

    // ------------------------------------------------------------------------
    //! \brief Drop the history and the target names. Called when the world is
    //! rebuilt, since the recorded actions no longer describe anything.
    // ------------------------------------------------------------------------
    void reset();

    // ------------------------------------------------------------------------
    //! \brief Message explaining what the armed tool expects, shown in the
    //! status bar.
    // ------------------------------------------------------------------------
    std::string hint() const;

private:

    // ------------------------------------------------------------------------
    //! \brief Make sure the target city, path, way type, unit type and map name
    //! still exist, and pick sensible ones when they do not. Called every frame
    //! because a script reload can replace all of them.
    // ------------------------------------------------------------------------
    void refreshTargets(Simulation& simulation);

    // ------------------------------------------------------------------------
    //! \brief The city the tools act on, or nullptr when there is none.
    // ------------------------------------------------------------------------
    City* targetCity(Simulation& simulation) const;

    // ------------------------------------------------------------------------
    //! \brief Round a world position to the grid of the target city, so that
    //! roads drawn by hand stay aligned.
    // ------------------------------------------------------------------------
    Vector3f snap(Simulation& simulation, ImVec2 const& world) const;

    void handleRoad(Simulation& simulation, ui::CityViewer& viewer, bool hovered);
    void handleBuilding(Simulation& simulation, ui::CityViewer& viewer);
    void handlePaint(Simulation& simulation, game::DebugState& state, bool hovered);
    void handleZone(Simulation& simulation, game::DebugState& state, bool hovered);
    void handleBulldozer(Simulation& simulation, ui::CityViewer& viewer);

private:

    EditTool m_tool = EditTool::Select;
    CommandStack m_stack;

    //! \brief Names of what the tools act on. Kept as names rather than
    //! pointers for the same reason the commands are.
    std::string m_city;
    std::string m_path;
    std::string m_wayType;
    std::string m_unitType;
    std::string m_map;
    std::string m_areaType;

    int m_paintAmount = 10;
    bool m_snapToGrid = true;

    //! \brief State of the drag in progress, for the road and the brush.
    bool m_dragging = false;
    Vector3f m_dragStart;
    Vector3f m_dragEnd;
    //! \brief Cell the brush started on, in world grid coordinates.
    int32_t m_dragU = 0;
    int32_t m_dragV = 0;
    int32_t m_dragU2 = 0;
    int32_t m_dragV2 = 0;
    bool m_dragValid = false;
};
} // namespace editor
} // namespace ogb

#endif
