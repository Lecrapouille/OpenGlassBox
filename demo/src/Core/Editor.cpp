//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Editor.hpp"
#include "UI/CityViewer.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ogb {
namespace core {
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
}

// ----------------------------------------------------------------------------
City* Editor::targetCity(Simulation& simulation) const
{
    auto const& cities = simulation.cities();
    auto it = cities.find(m_city);
    return (it == cities.end()) ? nullptr : it->second.get();
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
Vector3f Editor::snap(Simulation& simulation, ImVec2 const& world) const
{
    City* city = const_cast<Editor*>(this)->targetCity(simulation);
    if (!m_snapToGrid || (city == nullptr))
        return Vector3f(world.x, world.y, 0.0f);

    float const side = city->gridCellSize();
    float const x = city->position().x +
                    std::round((world.x - city->position().x) / side) * side;
    float const y = city->position().y +
                    std::round((world.y - city->position().y) / side) * side;

    return Vector3f(x, y, 0.0f);
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
        return "drag a rectangle to paint " + m_map;
    case EditTool::Zone:
        return "drag a rectangle to paint a " + m_areaType + " zone";
    case EditTool::Bulldozer:
        return "click a building or a road to demolish it";
    case EditTool::Select:
        break;
    }

    return {};
}

// ----------------------------------------------------------------------------
void Editor::drawToolbar(Simulation& simulation, DebugState& state)
{
    refreshTargets(simulation);

    struct Entry
    {
        EditTool tool;
        char const* label;
        char const* tooltip;
    };

    static Entry const ENTRIES[] = {
        { EditTool::Select, "Select",
          "Click to inspect a building, an agent, a node or a cell." },
        { EditTool::Road, "Road",
          "Drag between two points to lay a segment. The ends snap to the\n"
          "nearby nodes, so a road drawn across another one joins it." },
        { EditTool::Building, "Build",
          "Click on a road to place a building along it, without splitting\n"
          "the segment into extra nodes." },
        { EditTool::Zone, "Zone",
          "Drag a rectangle to paint an Area. Area rules then spawn,\n"
          "upgrade and destroy buildings inside it." },
        { EditTool::Paint, "Paint",
          "Drag a rectangle to set an amount of resource on the cells of a\n"
          "map. Handy to seed a simulation and watch the rules take over." },
        { EditTool::Bulldozer, "Bulldoze",
          "Click to demolish the building or the road under the cursor.\n"
          "The agents travelling on it are removed with it." },
    };

    for (auto const& entry: ENTRIES)
    {
        bool const active = (m_tool == entry.tool);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::ColorConvertU32ToFloat4(theme::ACCENT));
        }
        if (ImGui::Button(entry.label))
        {
            setTool(entry.tool);
        }
        if (active)
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", entry.tooltip);
        }
        ImGui::SameLine();
    }

    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_stack.canUndo());
    if (ImGui::Button("Undo"))
    {
        undo(simulation);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered() && m_stack.canUndo())
    {
        ImGui::SetTooltip("undo %s  (Ctrl+Z)", m_stack.undoLabel().c_str());
    }
    ImGui::SameLine();

    ImGui::BeginDisabled(!m_stack.canRedo());
    if (ImGui::Button("Redo"))
    {
        redo(simulation);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered() && m_stack.canRedo())
    {
        ImGui::SetTooltip("redo %s  (Ctrl+Y)", m_stack.redoLabel().c_str());
    }

    // Only the settings the armed tool actually uses, so the toolbar does not
    // turn into a wall of combos.
    City* city = targetCity(simulation);
    if ((m_tool == EditTool::Select) || (city == nullptr))
    {
        ImGui::Separator();
        return;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    nameCombo("##city", 90.0f, simulation.cities(), m_city);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("City the tool acts on.");
    }

    switch (m_tool)
    {
    case EditTool::Road:
        ImGui::SameLine();
        nameCombo("##path", 90.0f, city->paths(), m_path);
        ImGui::SameLine();
        nameCombo("##waytype", 110.0f, simulation.script().wayTypes(), m_wayType);
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &m_snapToGrid);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Round the ends to the grid of the city.");
        }
        break;

    case EditTool::Building:
        ImGui::SameLine();
        nameCombo("##unittype", 110.0f, simulation.script().unitTypes(), m_unitType);
        break;

    case EditTool::Zone:
        ImGui::SameLine();
        nameCombo("##areatype", 110.0f, simulation.script().areaTypes(), m_areaType);
        break;

    case EditTool::Paint:
        ImGui::SameLine();
        nameCombo("##map", 110.0f, city->maps(), m_map);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        {
            auto it = city->maps().find(m_map);
            int const capacity =
                (it == city->maps().end())
                    ? 100
                    : int(std::max(1u, it->second->getCapacity()));
            ImGui::SliderInt("##amount", &m_paintAmount, 0, capacity, "%d");
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Amount written on each cell of the rectangle.");
        }
        break;

    case EditTool::Select:
    case EditTool::Bulldozer:
        break;
    }

    ImGui::Separator();

    (void)state;
}

// ----------------------------------------------------------------------------
bool Editor::onCanvas(Simulation& simulation, DebugState& state,
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
        m_dragStart = snap(simulation, viewer.mouseWorld());
        m_dragEnd = m_dragStart;
        return;
    }

    if (!m_dragging)
        return;

    m_dragEnd = snap(simulation, viewer.mouseWorld());

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
void Editor::handlePaint(Simulation& simulation, DebugState& state, bool hovered)
{
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        state.hasHoveredCell && (state.hoveredCity == m_city))
    {
        m_dragging = true;
        m_dragValid = true;
        m_dragU = state.hoveredU;
        m_dragV = state.hoveredV;
        m_dragU2 = m_dragU;
        m_dragV2 = m_dragV;
        return;
    }

    if (!m_dragging)
        return;

    // Keep the last cell that was actually inside the city, so that dragging
    // past the edge clamps instead of cancelling the stroke.
    if (state.hasHoveredCell && (state.hoveredCity == m_city))
    {
        m_dragU2 = state.hoveredU;
        m_dragV2 = state.hoveredV;
    }

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        return;

    m_dragging = false;
    if (!m_dragValid)
        return;

    m_stack.push(simulation,
                 std::make_unique<PaintResourceCommand>(
                     m_city, m_map, m_dragU, m_dragV, m_dragU2, m_dragV2,
                     uint32_t(m_paintAmount)));
}

// ----------------------------------------------------------------------------
void Editor::handleZone(Simulation& simulation, DebugState& state, bool hovered)
{
    if (m_areaType.empty())
        return;

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        state.hasHoveredCell && (state.hoveredCity == m_city))
    {
        m_dragging = true;
        m_dragValid = true;
        m_dragU = state.hoveredU;
        m_dragV = state.hoveredV;
        m_dragU2 = m_dragU;
        m_dragV2 = m_dragV;
        return;
    }

    if (!m_dragging)
        return;

    if (state.hasHoveredCell && (state.hoveredCity == m_city))
    {
        m_dragU2 = state.hoveredU;
        m_dragV2 = state.hoveredV;
    }

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        return;

    m_dragging = false;
    if (!m_dragValid)
        return;

    m_stack.push(simulation,
                 std::make_unique<AddAreaCommand>(
                     m_city, m_areaType, int32_t(m_dragU), int32_t(m_dragV),
                     int32_t(m_dragU2), int32_t(m_dragV2)));
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
void Editor::drawPreview(Simulation& simulation, ui::CityViewer& viewer,
                         ImDrawList* drawList)
{
    City* city = targetCity(simulation);
    if ((city == nullptr) || (m_tool == EditTool::Select))
        return;

    ImU32 const preview = theme::ACCENT;

    switch (m_tool)
    {
    case EditTool::Road:
    {
        if (!m_dragging)
            break;

        ImVec2 const a = viewer.worldToScreen(m_dragStart);
        ImVec2 const b = viewer.worldToScreen(m_dragEnd);
        drawList->AddLine(a, b, preview, 3.0f);
        drawList->AddCircleFilled(a, 5.0f, preview);
        drawList->AddCircleFilled(b, 5.0f, preview);

        // The length matters: it is what the free flow travel time is computed
        // from, and therefore how attractive the new road will be.
        char label[48];
        std::snprintf(label, sizeof(label), "%.0f m",
                      magnitude(m_dragEnd - m_dragStart));
        drawList->AddText(ImVec2(b.x + 10.0f, b.y - 8.0f), preview, label);
        break;
    }
    case EditTool::Zone:
    case EditTool::Paint:
    {
        if (!m_dragging)
            break;

        int32_t const u0 = std::min(m_dragU, m_dragU2);
        int32_t const v0 = std::min(m_dragV, m_dragV2);
        int32_t const u1 = std::max(m_dragU, m_dragU2);
        int32_t const v1 = std::max(m_dragV, m_dragV2);

        ImVec2 const p0 = viewer.worldToScreen(
            city->world().mapPosition2world(u0, v0));
        ImVec2 const p1 = viewer.worldToScreen(
            city->world().mapPosition2world(u1 + 1, v1 + 1));

        drawList->AddRectFilled(p0, p1, theme::fromScript(0x56AADE, 0.20f));
        drawList->AddRect(p0, p1, preview, 0.0f, 0, 2.0f);

        char label[64];
        std::snprintf(label, sizeof(label), "%d x %d cells -> %d",
                      u1 - u0 + 1, v1 - v0 + 1, m_paintAmount);
        drawList->AddText(ImVec2(p0.x + 4.0f, p0.y - 18.0f), preview, label);
        break;
    }
    case EditTool::Building:
    case EditTool::Bulldozer:
    {
        // Show what the click would act on before it is made.
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
} // namespace core
} // namespace ogb
