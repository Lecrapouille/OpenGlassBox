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
    nameCombo("##areatype", 150.0f, simulation.script().areaTypes(), m_areaType);

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
        nameCombo("##map", 150.0f, city->maps(), m_map);
        if (!m_map.empty())
        {
            state.primaryLayer = m_map;
            state.layer(m_map).visible = true;
        }

        auto const it = city->maps().find(m_map);
        if (it != city->maps().end())
            capacity = int(std::max(1u, it->second->getCapacity()));
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

    // Which map to paint and which map to look at is the same choice, so the
    // layers hang off the Maps tool instead of a panel of their own.
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
    m_wayType.clear();
    m_unitType.clear();
    m_map.clear();
    m_areaType.clear();
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
    auto const& cities = simulation.cities();
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
    if (simulation.cities().find(m_city) == simulation.cities().end())
    {
        m_city = firstKey(simulation.cities());
    }

    City* city = targetCity(simulation);
    if (city == nullptr)
        return;

    if (city->paths().find(m_path) == city->paths().end())
    {
        m_path = firstKey(city->paths());
    }
    if (city->maps().find(m_map) == city->maps().end())
    {
        m_map = firstKey(city->maps());
    }
    if (simulation.script().wayTypes().find(m_wayType) == simulation.script().wayTypes().end())
    {
        m_wayType = firstKey(simulation.script().wayTypes());
    }
    if (simulation.script().unitTypes().find(m_unitType) == simulation.script().unitTypes().end())
    {
        m_unitType = firstKey(simulation.script().unitTypes());
    }
    if (simulation.script().areaTypes().find(m_areaType) == simulation.script().areaTypes().end())
    {
        m_areaType = firstKey(simulation.script().areaTypes());
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
            return node->position();

        float offset = 0.5f;
        Way* way = viewer.pickWay(*city, world, TOOL_PICK_PIXELS, offset);
        if (way != nullptr)
        {
            return way->position1() +
                   (way->position2() - way->position1()) * offset;
        }
    }

    if (!m_snapToGrid)
        return Vector3f(world.x, world.y, 0.0f);

    int32_t u = 0;
    int32_t v = 0;
    simulation.world().world2mapPosition(Vector3f(world.x, world.y, 0.0f), u, v);
    return simulation.world().mapPosition2world(u, v);
}

// ----------------------------------------------------------------------------
std::string Editor::hint() const
{
    switch (m_tool)
    {
    case EditTool::Road:
        return "drag to lay a " + m_wayType;
    case EditTool::Building:
        return "click a road to build a " + m_unitType;
    case EditTool::Paint:
        return "click or drag to paint " + m_map;
    case EditTool::Zone:
        return "click or drag to paint a " + m_areaType + " zone";
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
        m_city = city->name();

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
          "Click a road to place a building without splitting it." },
        { EditTool::Paint, "Maps",
          "Click or drag to write a resource on map cells, and pick which\n"
          "maps are drawn." },
        { EditTool::Bulldozer, "Demolish",
          "Click a building, road or orphan node. Clear empties the city." },
    };

    bool const hasAreas = !simulation.script().areaTypes().empty();
    bool const hasMaps = (city != nullptr) && !city->maps().empty();

    ImGui::BeginChild("ToolRail", ImVec2(96.0f, 0.0f), true);

    // Running the simulation and editing it are the two things the player does
    // with this rail, so Play sits on top of the tools rather than in a panel
    // on the other side of the window.
    bool const paused = simulation.paused();
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
            (entry.tool == EditTool::Zone && !hasAreas) ||
            (entry.tool == EditTool::Paint && !hasMaps);
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
            if (entry.tool == EditTool::Zone && !hasAreas)
                ImGui::SetTooltip("No zone in this ruleset.");
            else if (entry.tool == EditTool::Paint && !hasMaps)
                ImGui::SetTooltip("No map in this ruleset.");
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
        nameCombo("##waytype", 140.0f, simulation.script().wayTypes(), m_wayType);
        ImGui::SameLine();
        if (!city || city->paths().empty())
            nameCombo("##path", 100.0f, simulation.script().pathTypes(), m_path);
        else
            nameCombo("##path", 100.0f, city->paths(), m_path);
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
        nameCombo("##unittype", 160.0f, simulation.script().unitTypes(), m_unitType);
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
        ImGui::TextDisabled("Inspect: click the map");
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
                 std::make_unique<AddWayCommand>(m_city, m_path, m_wayType,
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
    if ((node != nullptr) && (node->path() != nullptr))
    {
        m_stack.push(simulation,
                     std::make_unique<AddUnitCommand>(
                         m_city, node->path()->type(), m_unitType, node->id()));
        return;
    }

    float offset = 0.5f;
    Way* way = viewer.pickWay(*city, world, TOOL_PICK_PIXELS, offset);
    if (way == nullptr)
        return;

    // Nudge the offset away from the extremities: a click in the middle of a
    // road should not snap onto an existing node.
    offset = std::min(0.95f, std::max(0.05f, offset));

    Path* path = way->from().path();
    if (path == nullptr)
        return;

    m_stack.push(simulation,
                 std::make_unique<AddUnitCommand>(m_city, path->type(),
                                                  m_unitType, way->id(), offset));
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
    if (m_map.empty())
        return;
    if (!trackBrushDrag(state, hovered))
        return;

    int32_t u0, v0, u1, v1;
    brushRectangle(u0, v0, u1, v1);

    if (!m_stack.push(simulation,
                      std::make_unique<PaintResourceCommand>(
                          m_city, m_map, u0, v0, u1, v1,
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
    if (m_areaType.empty())
        return;
    if (!trackBrushDrag(state, hovered))
        return;

    int32_t u0, v0, u1, v1;
    brushRectangle(u0, v0, u1, v1);

    auto command = std::make_unique<AddAreaCommand>(m_city, m_areaType,
                                                    u0, v0, u1, v1);
    AddAreaCommand const* const applied = command.get();
    if (!m_stack.push(simulation, std::move(command)))
        return;

    // The command stack owns the command now, so reading back the identifier
    // of the Area it created is safe.
    City* city = targetCity(simulation);
    if (city == nullptr)
        return;

    for (auto& area: city->areas())
    {
        if (area->id() != applied->createdAreaId())
            continue;

        state.selection.clear();
        state.selection.kind = game::Selection::Kind::Area;
        state.selection.city = m_city;
        state.selection.area = area.get();
        state.showAreas = true;
        break;
    }
}

// ----------------------------------------------------------------------------
void Editor::handleBulldozer(Simulation& simulation, ui::CityViewer& viewer)
{
    City* city = targetCity(simulation);
    ImVec2 const world = viewer.mouseWorld();

    // Buildings first: they sit on the road and would otherwise be impossible
    // to hit without also hitting the segment under them.
    Unit* unit = viewer.pickUnit(*city, world, TOOL_PICK_PIXELS);
    if (unit != nullptr)
    {
        Path* path = unit->path();
        if (path != nullptr)
        {
            if (unit->node() != nullptr)
            {
                m_stack.push(simulation,
                             std::make_unique<RemoveUnitCommand>(
                                 m_city, path->type(), unit->node()->id(),
                                 unit->type()));
            }
            else
            {
                m_stack.push(simulation,
                             std::make_unique<RemoveUnitCommand>(
                                 m_city, path->type(), unit->id(),
                                 unit->type(), true));
            }
        }
        else
        {
            m_stack.push(simulation,
                         std::make_unique<RemoveUnitCommand>(
                             m_city, "", unit->id(), unit->type(), true));
        }
        return;
    }

    Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
    if ((node != nullptr) && (node->path() != nullptr))
    {
        m_stack.push(simulation,
                     std::make_unique<RemoveNodeCommand>(
                         m_city, node->path()->type(), node->id()));
        return;
    }

    float offset = 0.0f;
    Way* way = viewer.pickWay(*city, world, TOOL_PICK_PIXELS, offset);
    if (way == nullptr)
        return;

    Path* path = way->from().path();
    if (path == nullptr)
        return;

    m_stack.push(simulation, std::make_unique<RemoveWayCommand>(
                                 m_city, path->type(), way->id()));
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
                          magnitude(m_dragEnd - m_dragStart));
            drawList->AddText(ImVec2(b.x + 10.0f, b.y - 8.0f), preview, label);
            break;
        }

        ImVec2 const world = viewer.mouseWorld();
        Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
        if (node != nullptr)
        {
            drawList->AddCircle(viewer.worldToScreen(node->position()),
                                8.0f, preview, 0, 2.5f);
            ImGui::SetTooltip("node #%u", node->id());
            break;
        }

        float offset = 0.5f;
        Way* way = viewer.pickWay(*city, world, TOOL_PICK_PIXELS, offset);
        if (way != nullptr)
        {
            Vector3f const p = way->position1() +
                               (way->position2() - way->position1()) * offset;
            drawList->AddLine(viewer.worldToScreen(way->position1()),
                              viewer.worldToScreen(way->position2()),
                              preview, 5.0f);
            drawList->AddCircleFilled(viewer.worldToScreen(p), 6.0f, preview);
            ImGui::SetTooltip("%s · %.0f m", way->type().c_str(),
                              way->magnitude());
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
                AreaType const& areaType =
                    simulation.script().getAreaType(m_areaType);
                fillColor = areaType.color;
                fillAlpha = 0.28f;
            }
            catch (...)
            {}
        }

        ImVec2 const p0 = viewer.worldToScreen(
            city->world().mapPosition2world(u0, v0));
        ImVec2 const p1 = viewer.worldToScreen(
            city->world().mapPosition2world(u1 + 1, v1 + 1));

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
                          u1 - u0 + 1, v1 - v0 + 1, m_areaType.c_str());
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
            Unit* unit = viewer.pickUnit(*city, world, TOOL_PICK_PIXELS);
            if (unit != nullptr)
            {
                drawList->AddCircle(viewer.worldToScreen(unit->position()),
                                    12.0f, theme::FAILURE, 0, 2.5f);
                break;
            }

            Node* node = viewer.pickNode(*city, world, TOOL_PICK_PIXELS);
            if (node != nullptr)
            {
                drawList->AddCircle(viewer.worldToScreen(node->position()),
                                    10.0f, theme::FAILURE, 0, 2.5f);
                break;
            }
        }

        float offset = 0.5f;
        Way* way = viewer.pickWay(*city, world, TOOL_PICK_PIXELS, offset);
        if (way == nullptr)
            break;

        ImU32 const color =
            (m_tool == EditTool::Bulldozer) ? theme::FAILURE : preview;
        drawList->AddLine(viewer.worldToScreen(way->position1()),
                          viewer.worldToScreen(way->position2()), color, 5.0f);

        if (m_tool == EditTool::Building)
        {
            Vector3f const p = way->position1() +
                               (way->position2() - way->position1()) * offset;
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
            "Nothing edited yet. Pick a tool in the toolbar of the map: every "
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
