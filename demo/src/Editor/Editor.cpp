//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Editor/Editor.hpp"
#include "UI/CityViewer.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ogb {
namespace editor {
using namespace ogb::theme;


//! \brief How close, in pixels, a click has to land on a road or a building for
//! a tool to act on it.
static constexpr float TOOL_PICK_PIXELS = 12.0f;
//! \brief How close, in world units, the end of a new road has to land on an
//! existing node to be joined to it instead of starting a new one.
static constexpr float SNAP_WORLD_RADIUS = 8.0f;

// ----------------------------------------------------------------------------
//! \brief Name of the first key of a map, or an empty string.
// ----------------------------------------------------------------------------
template<class Container>
static std::string firstKey(Container const& container)
{
    return container.empty() ? std::string() : container.begin()->first;
}

// ----------------------------------------------------------------------------
//! \brief Combo listing the keys of an associative container, writing the
//! chosen key back into \c current.
// ----------------------------------------------------------------------------
template<class Container>
static void nameCombo(char const* label, float width, Container const& container,
                      std::string& current)
{
    ImGui::SetNextItemWidth(width);
    if (!ImGui::BeginCombo(label, current.c_str()))
        return;

    for (auto const& it: container)
    {
        bool const selected = (it.first == current);
        if (ImGui::Selectable(it.first.c_str(), selected))
        {
            current = it.first;
        }
        if (selected)
        {
            ImGui::SetItemDefaultFocus();
        }
    }

    ImGui::EndCombo();
}

// ----------------------------------------------------------------------------
void Editor::drawBrushSlider(float width)
{
    ImGui::SetNextItemWidth(width);
    ImGui::SliderInt("##brush", &m_brush, 1, 32, "brush: %d");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "Side, in cells, of the square a single click covers.\n"
            "A drag covers the rectangle spanned by the brush at both ends.");
    }
}

// ----------------------------------------------------------------------------
//! \brief Width left on the row started by the last widget, or zero when the
//! next widget would not fit in it.
// ----------------------------------------------------------------------------
static float roomOnRow(float wanted)
{
    float const right = ImGui::GetWindowPos().x - ImGui::GetScrollX() +
                        ImGui::GetWindowContentRegionMax().x;
    float const left = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
    return ((right - left) >= wanted) ? (right - left) : 0.0f;
}

// ----------------------------------------------------------------------------
void Editor::drawZoneOptions(Simulation& simulation)
{
    nameCombo("##zonetype", 150.0f, simulation.getRuleset().getZoneTypes(), m_zoneType);

    float const room = roomOnRow(120.0f);
    if (room > 0.0f)
        ImGui::SameLine();
    drawBrushSlider((room > 0.0f) ? room : -1.0f);
}

// ----------------------------------------------------------------------------
void Editor::drawPaintOptions(Simulation& simulation, game::DebugState& state,
                              City* city)
{
    int capacity = 100;
    if (city != nullptr)
    {
        nameCombo("##layer", 150.0f, city->getLayers(), m_layer);

        // Show what is being painted, but only when the choice changes: doing
        // it every frame would pin that one layer visible and primary, and no
        // click in the list below could ever turn it off again.
        if (!m_layer.empty() && (m_layer != m_shownLayer))
        {
            m_shownLayer = m_layer;
            state.primaryLayer = m_layer;
            state.layer(m_layer).visible = true;
        }

        auto const it = city->getLayers().find(m_layer);
        if (it != city->getLayers().end())
            capacity = int(std::max(1u, it->second->getCellCapacity()));
    }

    float room = roomOnRow(140.0f);
    if (room > 0.0f)
        ImGui::SameLine();
    ImGui::SetNextItemWidth((room > 0.0f) ? room : -1.0f);
    ImGui::SliderInt("##amount", &m_paintAmount, 0, capacity, "value: %d");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Amount written on every cell of the brush.");

    room = roomOnRow(140.0f);
    if (room > 0.0f)
        ImGui::SameLine();
    drawBrushSlider((room > 0.0f) ? room : -1.0f);

    // Which layer to paint and which layer to look at is the same choice, so the
    // layers hang off the Layers tool instead of a panel of their own.
    ui::LayersPanel{}.drawColumn(simulation, state, -1.0f);
}

// ----------------------------------------------------------------------------
void Editor::brushRectangle(int32_t& u0, int32_t& v0, int32_t& u1,
                            int32_t& v1) const
{
    int32_t const half = (m_brush - 1) / 2;
    int32_t const rest = (m_brush - 1) - half;

    u0 = std::min(m_dragU, m_dragU2) - half;
    v0 = std::min(m_dragV, m_dragV2) - half;
    u1 = std::max(m_dragU, m_dragU2) + rest;
    v1 = std::max(m_dragV, m_dragV2) + rest;
}

// ----------------------------------------------------------------------------
void Editor::setTool(EditTool tool)
{
    m_tool = tool;
    m_dragging = false;
}

// ----------------------------------------------------------------------------
void Editor::reset()
{
    m_stack.clear();
    m_tool = EditTool::Select;
    m_dragging = false;
    m_city.clear();
    m_path.clear();
    m_segmentType.clear();
    m_buildingType.clear();
    m_layer.clear();
    m_shownLayer.clear();
    m_zoneType.clear();
}

// ----------------------------------------------------------------------------
void Editor::clearCity(Simulation& simulation, game::DebugState& state)
{
    City* city = targetCity(simulation);
    if (city == nullptr)
        return;

    city->clear();
    m_stack.clear();
    state.selection.clear();
}

// ----------------------------------------------------------------------------
City* Editor::targetCity(Simulation& simulation) const
{
    auto const& cities = simulation.getCities();
    if (cities.empty())
        return nullptr;
    if (!m_city.empty())
    {
        auto it = cities.find(m_city);
        if (it != cities.end())
            return it->second.get();
    }
    return cities.begin()->second.get();
}

// ----------------------------------------------------------------------------
void Editor::refreshTargets(Simulation& simulation)
{
    // Reloading a script replaces every type, so the names the editor holds are
    // checked against the live simulation rather than trusted.
    if (simulation.getCities().find(m_city) == simulation.getCities().end())
    {
        m_city = firstKey(simulation.getCities());
    }

    City* city = targetCity(simulation);
    if (city == nullptr)
        return;

    if (city->getPaths().find(m_path) == city->getPaths().end())
    {
        m_path = firstKey(city->getPaths());
        // An emptied or brand new city holds no graph yet: name the one the
        // ruleset declares, and laying a road will found it.
        if (m_path.empty())
            m_path = firstKey(simulation.getRuleset().getPathTypes());
    }
    if (city->getLayers().find(m_layer) == city->getLayers().end())
    {
        m_layer = firstKey(city->getLayers());
    }
    if (simulation.getRuleset().getSegmentTypes().find(m_segmentType) == simulation.getRuleset().getSegmentTypes().end())
    {
        m_segmentType = firstKey(simulation.getRuleset().getSegmentTypes());
    }
    if (simulation.getRuleset().getBuildingTypes().find(m_buildingType) == simulation.getRuleset().getBuildingTypes().end())
    {
        m_buildingType = firstKey(simulation.getRuleset().getBuildingTypes());
    }
    if (simulation.getRuleset().getZoneTypes().find(m_zoneType) == simulation.getRuleset().getZoneTypes().end())
    {
        m_zoneType = firstKey(simulation.getRuleset().getZoneTypes());
    }
}

// ----------------------------------------------------------------------------
Vector3f Editor::snap(Simulation& simulation, ui::CityViewer& viewer,
                      ImVec2 const& world) const
{
    City* city = targetCity(simulation);
    if (city != nullptr)
    {
        Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
        if (node != nullptr)
            return node->getPosition();

        float offset = 0.5f;
        Segment* segment = viewer.pickSegment(*city, world, TOOL_PICK_PIXELS, offset);
        if (segment != nullptr)
        {
            return segment->getFromPosition() +
                   (segment->getToPosition() - segment->getFromPosition()) * offset;
        }
    }

    if (!m_snapToGrid)
        return Vector3f(world.x, world.y, 0.0f);

    return simulation.cellToWorld(
        simulation.worldToCell({ world.x, world.y, 0.0f }));
}

// ----------------------------------------------------------------------------
std::string Editor::hint() const
{
    switch (m_tool)
    {
    case EditTool::Road:
        return "drag to lay a " + m_segmentType;
    case EditTool::Building:
        return "click a road to build a " + m_buildingType;
    case EditTool::Paint:
        return "click or drag to paint " + m_layer;
    case EditTool::Zone:
        return "click or drag to paint a " + m_zoneType + " zone";
    case EditTool::Bulldozer:
        return "click a building, a road or a node";
    case EditTool::Select:
        break;
    }

    return {};
}

// ----------------------------------------------------------------------------
void Editor::drawToolbar(Simulation& simulation, game::DebugState& state)
{
    refreshTargets(simulation);
    City* city = targetCity(simulation);
    if (city != nullptr)
        m_city = city->getName();

    struct Entry
    {
        EditTool tool;
        char const* label;
        char const* tooltip;
    };

    static Entry const ENTRIES[] = {
        { EditTool::Select, "Inspect",
          "Click a building, agent, road, node or cell." },
        { EditTool::Road, "Roads",
          "Drag to lay a road. Ends snap to the world grid and to nearby nodes." },
        { EditTool::Zone, "Zones",
          "Click or drag to paint a zone. Its rules grow buildings inside it.\n"
          "Painting over another zone re-zones only the cells you paint." },
        { EditTool::Building, "Buildings",
          "Click a road to place a building. The segment is cut in two and\n"
          "the building sits on the junction, where agents can stop." },
        { EditTool::Paint, "Layers",
          "Click or drag to write a resource on layer cells, and pick which\n"
          "layers are drawn." },
        { EditTool::Bulldozer, "Demolish",
          "Click a building, road or orphan node. Clear empties the city." },
    };

    bool const hasZones = !simulation.getRuleset().getZoneTypes().empty();
    bool const hasLayers = (city != nullptr) && !city->getLayers().empty();

    ImGui::BeginChild("ToolRail", ImVec2(96.0f, 0.0f), true);

    // Running the simulation and editing it are the two things the player does
    // with this rail, so Play sits on top of the tools rather than in a panel
    // on the other side of the window.
    bool const paused = simulation.isPaused();
    if (!paused)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::ColorConvertU32ToFloat4(theme::SUCCESS));
    }
    if (ImGui::Button(paused ? "Play" : "Pause", ImVec2(-1.0f, 0.0f)))
        simulation.setPaused(!paused);
    if (!paused)
        ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Run or hold the simulation. Shortcut: space.");
    ImGui::Separator();

    for (auto const& entry: ENTRIES)
    {
        bool const disabled =
            (entry.tool == EditTool::Zone && !hasZones) ||
            (entry.tool == EditTool::Paint && !hasLayers);
        bool const active = (m_tool == entry.tool);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::ColorConvertU32ToFloat4(theme::ACCENT));
        }
        ImGui::BeginDisabled(disabled);
        if (ImGui::Button(entry.label, ImVec2(-1.0f, 0.0f)))
            setTool(entry.tool);
        ImGui::EndDisabled();
        if (active)
            ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            if (entry.tool == EditTool::Zone && !hasZones)
                ImGui::SetTooltip("No zone in this ruleset.");
            else if (entry.tool == EditTool::Paint && !hasLayers)
                ImGui::SetTooltip("No layer in this ruleset.");
            else
                ImGui::SetTooltip("%s", entry.tooltip);
        }
    }

    ImGui::Separator();
    ImGui::BeginDisabled(!m_stack.canUndo());
    if (ImGui::Button("Undo", ImVec2(-1.0f, 0.0f)))
        undo(simulation);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!m_stack.canRedo());
    if (ImGui::Button("Redo", ImVec2(-1.0f, 0.0f)))
        redo(simulation);
    ImGui::EndDisabled();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginGroup();

    switch (m_tool)
    {
    case EditTool::Road:
        nameCombo("##segmenttype", 140.0f, simulation.getRuleset().getSegmentTypes(), m_segmentType);
        ImGui::SameLine();
        if (!city || city->getPaths().empty())
            nameCombo("##path", 100.0f, simulation.getRuleset().getPathTypes(), m_path);
        else
            nameCombo("##path", 100.0f, city->getPaths(), m_path);
        ImGui::SameLine();
        ImGui::Checkbox("Snap grid", &m_snapToGrid);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Align freehand road ends to the city grid.\n"
                "Roads and nodes under the cursor are always preferred.");
        }
        ImGui::SameLine();
        ImGui::Checkbox("Traffic colors", &state.showTraffic);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Color roads by congestion (free to jammed).");
        break;
    case EditTool::Building:
        nameCombo("##buildingtype", 160.0f, simulation.getRuleset().getBuildingTypes(), m_buildingType);
        break;
    case EditTool::Zone:
        drawZoneOptions(simulation);
        break;
    case EditTool::Paint:
        drawPaintOptions(simulation, state, city);
        break;
    case EditTool::Bulldozer:
        ImGui::TextDisabled("Click a building, road or node");
        ImGui::SameLine();
        if (ImGui::Button("Clear city"))
        {
            clearCity(simulation, state);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Remove every road, building, agent and zone.\n"
                              "The ruleset stays. This cannot be undone.");
        }
        break;
    case EditTool::Select:
        ImGui::TextDisabled("Inspect: click the layer");
        break;
    }

}

// ----------------------------------------------------------------------------
bool Editor::onCanvas(Simulation& simulation, game::DebugState& state,
                      ui::CityViewer& viewer, bool hovered)
{
    // The shortcuts work whatever the armed tool: undoing is not an edit.
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        undo(simulation);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        redo(simulation);
    }

    if (m_tool == EditTool::Select)
        return false;

    if (targetCity(simulation) == nullptr)
        return false;

    switch (m_tool)
    {
    case EditTool::Road:
        handleRoad(simulation, viewer, hovered);
        break;
    case EditTool::Building:
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            handleBuilding(simulation, viewer);
        }
        break;
    case EditTool::Paint:
        handlePaint(simulation, state, hovered);
        break;
    case EditTool::Zone:
        handleZone(simulation, state, hovered);
        break;
    case EditTool::Bulldozer:
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            handleBulldozer(simulation, viewer);
        }
        break;
    case EditTool::Select:
        break;
    }

    return true;
}

// ----------------------------------------------------------------------------
void Editor::handleRoad(Simulation& simulation, ui::CityViewer& viewer, bool hovered)
{
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_dragging = true;
        m_dragStart = snap(simulation, viewer, viewer.mouseWorld());
        m_dragEnd = m_dragStart;
        return;
    }

    if (!m_dragging)
        return;

    m_dragEnd = snap(simulation, viewer, viewer.mouseWorld());

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        return;

    m_dragging = false;

    m_stack.push(simulation,
                 std::make_unique<AddSegmentCommand>(m_city, m_path, m_segmentType,
                                                 m_dragStart, m_dragEnd,
                                                 SNAP_WORLD_RADIUS));
}

// ----------------------------------------------------------------------------
void Editor::handleBuilding(Simulation& simulation, ui::CityViewer& viewer)
{
    City* city = targetCity(simulation);
    ImVec2 const world = viewer.mouseWorld();

    // A click right on a node builds there rather than projecting onto one of
    // its segments a hair away from it.
    Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
    if ((node != nullptr) && (node->getPath() != nullptr))
    {
        m_stack.push(simulation,
                     std::make_unique<AddBuildingCommand>(
                         m_city, node->getPath()->getTypeName().str(), m_buildingType, node->getId()));
        return;
    }

    float offset = 0.5f;
    Segment* segment = viewer.pickSegment(*city, world, TOOL_PICK_PIXELS, offset);
    if (segment == nullptr)
        return;

    // Nudge the offset away from the extremities: a click in the middle of a
    // road should not snap onto an existing node.
    offset = std::min(0.95f, std::max(0.05f, offset));

    Path* path = segment->getFrom().getPath();
    if (path == nullptr)
        return;

    m_stack.push(simulation,
                 std::make_unique<AddBuildingCommand>(m_city, path->getTypeName().str(),
                                                  m_buildingType, segment->getId(), offset));
}

// ----------------------------------------------------------------------------
bool Editor::trackBrushDrag(game::DebugState& state, bool hovered)
{
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        state.hasHoveredCell)
    {
        m_dragging = true;
        m_dragValid = true;
        m_dragU = state.hoveredU;
        m_dragV = state.hoveredV;
        m_dragU2 = m_dragU;
        m_dragV2 = m_dragV;
        return false;
    }

    if (!m_dragging)
        return false;

    // Keep the last cell that was actually inside the city, so that dragging
    // past the edge clamps instead of cancelling the stroke.
    if (state.hasHoveredCell)
    {
        m_dragU2 = state.hoveredU;
        m_dragV2 = state.hoveredV;
    }

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        return false;

    // The stroke is committed on release only: a plain click and a drag then
    // go through the very same command, instead of the click applying a first
    // one-cell edit that the release would immediately stack a second one on.
    m_dragging = false;
    return m_dragValid;
}

// ----------------------------------------------------------------------------
void Editor::handlePaint(Simulation& simulation, game::DebugState& state, bool hovered)
{
    if (m_layer.empty())
        return;
    if (!trackBrushDrag(state, hovered))
        return;

    int32_t u0, v0, u1, v1;
    brushRectangle(u0, v0, u1, v1);

    if (!m_stack.push(simulation,
                      std::make_unique<PaintResourceCommand>(
                          m_city, m_layer, u0, v0, u1, v1,
                          uint32_t(m_paintAmount))))
    {
        return;
    }

    // Show the result in the Inspector: a single cell of a large city is a
    // fraction of a pixel on screen, so the numbers are the only feedback that
    // is readable at any zoom.
    state.selection.clear();
    state.selection.kind = game::Selection::Kind::Cell;
    state.selection.city = m_city;
    state.selection.u = m_dragU;
    state.selection.v = m_dragV;
}

// ----------------------------------------------------------------------------
void Editor::handleZone(Simulation& simulation, game::DebugState& state, bool hovered)
{
    if (m_zoneType.empty())
        return;
    if (!trackBrushDrag(state, hovered))
        return;

    int32_t u0, v0, u1, v1;
    brushRectangle(u0, v0, u1, v1);

    auto command = std::make_unique<AddZoneCommand>(m_city, m_zoneType,
                                                    u0, v0, u1, v1);
    if (!m_stack.push(simulation, std::move(command)))
        return;

    // A zone wears the colour of its type, so selecting the one just painted
    // added nothing but a highlight the player had no way to dismiss: the last
    // rectangle drawn stayed blue for the rest of the session.
    state.showZones = true;
}

// ----------------------------------------------------------------------------
void Editor::handleBulldozer(Simulation& simulation, ui::CityViewer& viewer)
{
    City* city = targetCity(simulation);
    ImVec2 const world = viewer.mouseWorld();

    // Buildings first: they sit on the road and would otherwise be impossible
    // to hit without also hitting the segment under them.
    Building* building = viewer.pickBuilding(*city, world, TOOL_PICK_PIXELS);
    if (building != nullptr)
    {
        Path* path = building->getPath();
        if (path != nullptr)
        {
            if (building->getNode() != nullptr)
            {
                m_stack.push(simulation,
                             std::make_unique<RemoveBuildingCommand>(
                                 m_city, path->getTypeName().str(), building->getNode()->getId(),
                                 building->getTypeName().str()));
            }
            else
            {
                m_stack.push(simulation,
                             std::make_unique<RemoveBuildingCommand>(
                                 m_city, path->getTypeName().str(), building->getId(),
                                 building->getTypeName().str(), true));
            }
        }
        else
        {
            m_stack.push(simulation,
                         std::make_unique<RemoveBuildingCommand>(
                             m_city, "", building->getId(), building->getTypeName().str(), true));
        }
        return;
    }

    Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
    if ((node != nullptr) && (node->getPath() != nullptr))
    {
        m_stack.push(simulation,
                     std::make_unique<RemoveNodeCommand>(
                         m_city, node->getPath()->getTypeName().str(), node->getId()));
        return;
    }

    float offset = 0.0f;
    Segment* segment = viewer.pickSegment(*city, world, TOOL_PICK_PIXELS, offset);
    if (segment == nullptr)
        return;

    Path* path = segment->getFrom().getPath();
    if (path == nullptr)
        return;

    m_stack.push(simulation, std::make_unique<RemoveSegmentCommand>(
                                 m_city, path->getTypeName().str(), segment->getId()));
}

// ----------------------------------------------------------------------------
void Editor::drawPreview(Simulation& simulation, game::DebugState& state,
                         ui::CityViewer& viewer, ImDrawList* drawList)
{
    City* city = targetCity(simulation);
    if ((city == nullptr) || (m_tool == EditTool::Select))
        return;

    ImU32 const preview = theme::ACCENT;

    switch (m_tool)
    {
    case EditTool::Road:
    {
        if (m_dragging)
        {
            ImVec2 const a = viewer.worldToScreen(m_dragStart);
            ImVec2 const b = viewer.worldToScreen(m_dragEnd);
            drawList->AddLine(a, b, preview, 3.0f);
            drawList->AddCircleFilled(a, 5.0f, preview);
            drawList->AddCircleFilled(b, 5.0f, preview);

            char label[48];
            std::snprintf(label, sizeof(label), "%.0f m",
                          length(m_dragEnd - m_dragStart));
            drawList->AddText(ImVec2(b.x + 10.0f, b.y - 8.0f), preview, label);
            break;
        }

        ImVec2 const world = viewer.mouseWorld();
        Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
        if (node != nullptr)
        {
            drawList->AddCircle(viewer.worldToScreen(node->getPosition()),
                                8.0f, preview, 0, 2.5f);
            ImGui::SetTooltip("node #%u", node->getId());
            break;
        }

        float offset = 0.5f;
        Segment* segment = viewer.pickSegment(*city, world, TOOL_PICK_PIXELS, offset);
        if (segment != nullptr)
        {
            Vector3f const p = segment->getFromPosition() +
                               (segment->getToPosition() - segment->getFromPosition()) * offset;
            drawList->AddLine(viewer.worldToScreen(segment->getFromPosition()),
                              viewer.worldToScreen(segment->getToPosition()),
                              preview, 5.0f);
            drawList->AddCircleFilled(viewer.worldToScreen(p), 6.0f, preview);
            ImGui::SetTooltip("%s · %.0f m", segment->getTypeName().c_str(),
                              segment->getLength());
            break;
        }

        // Over open ground, show the node the click would create, at the very
        // place the snapping would put it.
        Vector3f const snapped = snap(simulation, viewer, world);
        ImVec2 const centre = viewer.worldToScreen(snapped);
        drawList->AddCircleFilled(centre, 4.0f, preview);
        drawList->AddCircle(centre, 9.0f, preview, 0, 1.5f);
        break;
    }
    case EditTool::Zone:
    case EditTool::Paint:
    {
        int32_t u0 = 0;
        int32_t v0 = 0;
        int32_t u1 = 0;
        int32_t v1 = 0;
        bool show = false;

        if (m_dragging)
        {
            brushRectangle(u0, v0, u1, v1);
            show = true;
        }
        else if (state.hasHoveredCell)
        {
            int32_t const half = (m_brush - 1) / 2;
            int32_t const rest = (m_brush - 1) - half;
            u0 = state.hoveredU - half;
            v0 = state.hoveredV - half;
            u1 = state.hoveredU + rest;
            v1 = state.hoveredV + rest;
            show = true;
        }

        if (!show)
            break;

        uint32_t fillColor = 0x56AADE;
        float fillAlpha = 0.20f;
        if (m_tool == EditTool::Zone)
        {
            try
            {
                ZoneType const& zoneType =
                    simulation.getRuleset().getZoneType(m_zoneType);
                fillColor = zoneType.color;
                fillAlpha = 0.28f;
            }
            catch (...)
            {}
        }

        ImVec2 const p0 = viewer.worldToScreen(
            city->cellToWorld({ u0, v0 }));
        ImVec2 const p1 = viewer.worldToScreen(
            city->cellToWorld({ u1 + 1, v1 + 1 }));

        drawList->AddRectFilled(p0, p1, theme::fromScript(fillColor, fillAlpha));
        drawList->AddRect(p0, p1, preview, 0.0f, 0, 2.0f);

        char label[64];
        if (m_tool == EditTool::Paint)
        {
            std::snprintf(label, sizeof(label), "%d x %d cells -> %d",
                          u1 - u0 + 1, v1 - v0 + 1, m_paintAmount);
        }
        else
        {
            std::snprintf(label, sizeof(label), "%d x %d cells -> %s",
                          u1 - u0 + 1, v1 - v0 + 1, m_zoneType.c_str());
        }
        drawList->AddText(ImVec2(p0.x + 4.0f, p0.y - 18.0f), preview, label);
        break;
    }
    case EditTool::Building:
    case EditTool::Bulldozer:
    {
        ImVec2 const world = viewer.mouseWorld();

        if (m_tool == EditTool::Bulldozer)
        {
            Building* building = viewer.pickBuilding(*city, world, TOOL_PICK_PIXELS);
            if (building != nullptr)
            {
                drawList->AddCircle(viewer.worldToScreen(building->getPosition()),
                                    12.0f, theme::FAILURE, 0, 2.5f);
                break;
            }

            Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
            if (node != nullptr)
            {
                drawList->AddCircle(viewer.worldToScreen(node->getPosition()),
                                    10.0f, theme::FAILURE, 0, 2.5f);
                break;
            }
        }

        float offset = 0.5f;
        Segment* segment = viewer.pickSegment(*city, world, TOOL_PICK_PIXELS, offset);
        if (segment == nullptr)
            break;

        ImU32 const color =
            (m_tool == EditTool::Bulldozer) ? theme::FAILURE : preview;
        drawList->AddLine(viewer.worldToScreen(segment->getFromPosition()),
                          viewer.worldToScreen(segment->getToPosition()), color, 5.0f);

        if (m_tool == EditTool::Building)
        {
            Vector3f const p = segment->getFromPosition() +
                               (segment->getToPosition() - segment->getFromPosition()) * offset;
            drawList->AddCircleFilled(viewer.worldToScreen(p), 6.0f, preview);
        }
        break;
    }
    case EditTool::Select:
        break;
    }
}

// ----------------------------------------------------------------------------
void Editor::drawHistoryPanel(Simulation& simulation)
{
    if (!ImGui::Begin("History"))
    {
        ImGui::End();
        return;
    }

    ImGui::BeginDisabled(!m_stack.canUndo());
    if (ImGui::Button("Undo"))
    {
        undo(simulation);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_stack.canRedo());
    if (ImGui::Button("Redo"))
    {
        redo(simulation);
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    auto const& history = m_stack.history();
    if (history.empty() && (m_stack.pendingRedos() == 0u))
    {
        ImGui::TextWrapped(
            "Nothing edited yet. Pick a tool in the toolbar of the layer: every "
            "road laid, building placed, cell painted and demolition lands "
            "here and can be taken back.");
        ImGui::End();
        return;
    }

    if (ImGui::BeginChild("entries"))
    {
        size_t index = history.size();
        for (auto it = history.rbegin(); it != history.rend(); ++it)
        {
            ImGui::Text("%zu.", index--);
            ImGui::SameLine();
            ImGui::TextUnformatted((*it)->label().c_str());
        }

        if (m_stack.pendingRedos() != 0u)
        {
            ImGui::Separator();
            ImGui::TextDisabled("%zu action(s) undone, still redoable",
                                m_stack.pendingRedos());
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
} // namespace editor
} // namespace ogb
