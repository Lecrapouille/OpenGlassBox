//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/CityViewer.hpp"
#include "Editor/Editor.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace ogb
{
namespace ui
{
using namespace ogb::theme;

//! \brief Bounds of the zoom, in pixels per world unit.
static constexpr float MIN_ZOOM = 0.05f;
static constexpr float MAX_ZOOM = 20.0f;
//! \brief How far, in pixels, a click may land from an entity to select it.
static constexpr float PICK_RADIUS = 12.0f;
//! \brief Below that many pixels a grid cell is not worth a rectangle of its
//! own, and the maps are drawn by squares of several cells instead.
static constexpr float MIN_CELL_PIXELS = 3.0f;
//! \brief How many squares one layer may put on screen. Past that the squares
//! are made to cover more cells each, so that the cost of drawing a layer
//! follows the size of the canvas rather than the size of the city.
static constexpr uint64_t MAX_MAP_SQUARES = 20000u;
//! \brief Radius, in pixels, of a Node and of a Unit at zoom one.
static constexpr float NODE_RADIUS = 4.0f;
static constexpr float UNIT_RADIUS = 7.0f;
static constexpr float AGENT_RADIUS = 3.5f;
//! \brief Below this zoom, the labels and the small details are dropped: they
//! would be unreadable and cost a lot of vertices.
static constexpr float LABEL_ZOOM_THRESHOLD = 0.45f;
static constexpr float DETAIL_ZOOM_THRESHOLD = 0.15f;

// ----------------------------------------------------------------------------
ImVec2 CityViewer::worldToScreen(float x, float y) const
{
    return ImVec2(m_canvas_origin.x + m_offset.x + x * m_zoom,
                  m_canvas_origin.y + m_offset.y + y * m_zoom);
}

// ----------------------------------------------------------------------------
ImVec2 CityViewer::worldToScreen(Vector3f const& world) const
{
    return worldToScreen(world.x, world.y);
}

// ----------------------------------------------------------------------------
ImVec2 CityViewer::screenToWorld(ImVec2 const& screen) const
{
    return ImVec2((screen.x - m_canvas_origin.x - m_offset.x) / m_zoom,
                  (screen.y - m_canvas_origin.y - m_offset.y) / m_zoom);
}

// ----------------------------------------------------------------------------
void CityViewer::frameAll(Simulation& simulation)
{
    if ((m_canvas_size.x <= 0.0f) || (m_canvas_size.y <= 0.0f))
        return;

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    bool found = false;

    for (auto const& it : simulation.cities())
    {
        City const& city = *it.second;

        // The grid of the city always bounds its maps, and the nodes may stick
        // out of it, so take the union of both.
        float const side = city.gridCellSize();
        minX = std::min(minX, city.position().x);
        minY = std::min(minY, city.position().y);
        maxX =
            std::max(maxX, city.position().x + float(city.gridSizeU()) * side);
        maxY =
            std::max(maxY, city.position().y + float(city.gridSizeV()) * side);
        found = true;

        for (auto const& path : city.paths())
        {
            for (auto const& node : path.second->nodes())
            {
                minX = std::min(minX, node->position().x);
                minY = std::min(minY, node->position().y);
                maxX = std::max(maxX, node->position().x);
                maxY = std::max(maxY, node->position().y);
            }
        }
    }

    if (!found)
    {
        m_offset = ImVec2(0.0f, 0.0f);
        m_zoom = 1.0f;
        return;
    }

    float const margin = 40.0f;
    float const width = std::max(1.0f, maxX - minX);
    float const height = std::max(1.0f, maxY - minY);

    m_zoom = std::min((m_canvas_size.x - 2.0f * margin) / width,
                      (m_canvas_size.y - 2.0f * margin) / height);
    m_zoom = std::min(MAX_ZOOM, std::max(MIN_ZOOM, m_zoom));

    m_offset.x = 0.5f * m_canvas_size.x - 0.5f * (minX + maxX) * m_zoom;
    m_offset.y = 0.5f * m_canvas_size.y - 0.5f * (minY + maxY) * m_zoom;
}

// ----------------------------------------------------------------------------
void CityViewer::draw(Simulation& simulation,
                      game::DebugState& state,
                      editor::Editor& editor)
{
    if (!ImGui::Begin("Map"))
    {
        ImGui::End();
        return;
    }

    drawToolbar(simulation, state, editor);

    SimulationClock const& clock = simulation.clock();
    float const hour =
        float(clock.hourOfDay()) + float(clock.minuteOfHour()) / 60.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::canvasBackground(hour));
    ImGui::BeginChild("Canvas",
                      ImVec2(0.0f, 0.0f),
                      true,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse |
                          ImGuiWindowFlags_NoMove);

    m_canvas_origin = ImGui::GetCursorScreenPos();
    m_canvas_size = ImGui::GetContentRegionAvail();
    m_draw_list = ImGui::GetWindowDrawList();

    // Wait until the canvas has a usable size. On the first frames the panel is
    // still being laid out by the dockspace, and framing against a few pixels
    // would leave the view stuck at the minimum zoom.
    if ((m_frames_to_reframe > 0) && (m_canvas_size.x > 100.0f) &&
        (m_canvas_size.y > 100.0f))
    {
        frameAll(simulation);
        --m_frames_to_reframe;
    }

    // Clip everything to the canvas, otherwise a city far outside the view
    // would spill over the neighboring panels.
    m_draw_list->PushClipRect(
        m_canvas_origin, m_canvas_origin + m_canvas_size, true);

    handleInputs(simulation, state, editor);

    m_splitter.Split(m_draw_list, CHANNEL_COUNT);

    // The maps belong to the world and span every city, so they are drawn once
    // rather than once per city.
    drawMaps(simulation.world(), state);

    for (auto& it : simulation.cities())
    {
        City& city = *it.second;
        drawCityFrame(city, state);
        if (state.showAreas)
            drawAreas(city);
        drawPaths(city, state);
        drawUnits(city, state);
        drawAgents(city, state);
    }

    drawSelectionOverlay(simulation, state, editor);

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_OVERLAY);
    editor.drawPreview(simulation, state, *this, m_draw_list);

    m_splitter.Merge(m_draw_list);

    m_draw_list->PopClipRect();

    drawLegend(state);
    drawHoverTooltip(simulation, state, editor);
    drawClockHud(simulation);
    drawHint(editor);

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::End();
}

// ----------------------------------------------------------------------------
void CityViewer::drawToolbar(Simulation& simulation,
                             game::DebugState& state,
                             editor::Editor& editor)
{
    editor.drawToolbar(simulation, state);

    if (ImGui::Button("Recenter"))
    {
        requestFrameAll();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Frame the whole city in the view. Shortcut: Home.");
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    ImGui::SliderFloat("Zoom",
                       &m_zoom,
                       MIN_ZOOM,
                       MAX_ZOOM,
                       "%.2f",
                       ImGuiSliderFlags_Logarithmic);

    drawDisplayToggles(state);

    ImGui::Separator();
}

// ----------------------------------------------------------------------------
void CityViewer::handleInputs(Simulation& simulation,
                              game::DebugState& state,
                              editor::Editor& editor)
{
    // The child window is not a widget, so claim the area explicitly to know
    // whether the mouse belongs to us.
    ImGui::InvisibleButton("CanvasSurface",
                           m_canvas_size,
                           ImGuiButtonFlags_MouseButtonLeft |
                               ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);

    bool const hovered = ImGui::IsItemHovered();
    ImGuiIO const& io = ImGui::GetIO();

    // Pan with the middle button, or with the right one which is easier to
    // reach on a laptop.
    if (ImGui::IsItemActive() &&
        (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
         ImGui::IsMouseDragging(ImGuiMouseButton_Right)))
    {
        m_offset.x += io.MouseDelta.x;
        m_offset.y += io.MouseDelta.y;
    }

    if (hovered && (io.MouseWheel != 0.0f))
    {
        // Keep the world point under the cursor pinned while zooming.
        ImVec2 const before = screenToWorld(io.MousePos);

        m_zoom *= std::pow(1.15f, io.MouseWheel);
        m_zoom = std::min(MAX_ZOOM, std::max(MIN_ZOOM, m_zoom));

        ImVec2 const after = screenToWorld(io.MousePos);
        m_offset.x += (after.x - before.x) * m_zoom;
        m_offset.y += (after.y - before.y) * m_zoom;
    }

    // The hovered cell drives both the tooltip and the brush, and the tools
    // read it, so refresh it before they run.
    updateHover(simulation, state, hovered || ImGui::IsItemActive());

    // An armed tool owns the left button. Only when it declines does the click
    // fall through to the selection.
    if (editor.onCanvas(simulation, state, *this, hovered))
        return;

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        pick(simulation, state, io.MousePos);
    }
}

// ----------------------------------------------------------------------------
//! \brief Squared distance from a point to a segment, and where the projection
//! lands along it.
// ----------------------------------------------------------------------------
static float distanceToSegment(ImVec2 const& point,
                               Vector3f const& a,
                               Vector3f const& b,
                               float& offset)
{
    float const abx = b.x - a.x;
    float const aby = b.y - a.y;
    float const length2 = abx * abx + aby * aby;

    if (length2 <= 0.0f)
    {
        offset = 0.0f;
        float const dx = point.x - a.x;
        float const dy = point.y - a.y;
        return dx * dx + dy * dy;
    }

    float t = ((point.x - a.x) * abx + (point.y - a.y) * aby) / length2;
    t = std::min(1.0f, std::max(0.0f, t));
    offset = t;

    float const dx = point.x - (a.x + t * abx);
    float const dy = point.y - (a.y + t * aby);

    return dx * dx + dy * dy;
}

// ----------------------------------------------------------------------------
Way* CityViewer::pickWay(City& city,
                         ImVec2 const& world,
                         float pixels,
                         float& offset) const
{
    // Work in world units so that the tolerance stays the same on screen
    // whatever the zoom.
    float const tolerance = pixels / std::max(1e-3f, m_zoom);
    float best = tolerance * tolerance;
    Way* found = nullptr;

    for (auto const& it : city.paths())
    {
        for (auto& way : it.second->ways())
        {
            float candidate = 0.0f;
            float const distance = distanceToSegment(
                world, way->position1(), way->position2(), candidate);
            if (distance <= best)
            {
                best = distance;
                found = way.get();
                offset = candidate;
            }
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
Node* CityViewer::pickNode(City& city, ImVec2 const& world, float pixels) const
{
    float const tolerance = pixels / std::max(1e-3f, m_zoom);
    float best = tolerance * tolerance;
    Node* found = nullptr;

    for (auto const& it : city.paths())
    {
        for (auto& node : it.second->nodes())
        {
            float const dx = node->position().x - world.x;
            float const dy = node->position().y - world.y;
            float const distance = dx * dx + dy * dy;
            if (distance <= best)
            {
                best = distance;
                found = node.get();
            }
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
Unit* CityViewer::pickUnit(City& city, ImVec2 const& world, float pixels) const
{
    float const tolerance = pixels / std::max(1e-3f, m_zoom);
    float best = tolerance * tolerance;
    Unit* found = nullptr;

    for (auto const& unit : city.units())
    {
        float const dx = unit->position().x - world.x;
        float const dy = unit->position().y - world.y;
        float const distance = dx * dx + dy * dy;
        if (distance <= best)
        {
            best = distance;
            found = unit.get();
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
Agent*
CityViewer::pickAgent(City& city, ImVec2 const& world, float pixels) const
{
    float const tolerance = pixels / std::max(1e-3f, m_zoom);
    float best = tolerance * tolerance;
    Agent* found = nullptr;

    for (auto const& agent : city.agents())
    {
        float const dx = agent->position().x - world.x;
        float const dy = agent->position().y - world.y;
        float const distance = dx * dx + dy * dy;
        if (distance <= best)
        {
            best = distance;
            found = agent.get();
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
game::Selection CityViewer::pickAt(Simulation& simulation,
                                   game::DebugState const& state,
                                   ImVec2 const& screen) const
{
    float const reach = PICK_RADIUS * PICK_RADIUS;
    float bestDistance = std::numeric_limits<float>::infinity();
    game::Selection best;

    // Candidates are offered building first, then agent, then node, and a tie
    // goes to the first of them. A building dropped on a junction shares the
    // position of its node to the pixel, and letting the node win is what made
    // every house placed by the Buildings tool look like a bare node.
    auto const consider = [&](ImVec2 const& position, game::Selection candidate)
    {
        float const dx = position.x - screen.x;
        float const dy = position.y - screen.y;
        float const distance = dx * dx + dy * dy;
        if ((distance > reach) || (distance >= bestDistance))
            return;

        bestDistance = distance;
        best = std::move(candidate);
    };

    for (auto& it : simulation.cities())
    {
        City& city = *it.second;

        if (state.showUnits)
        {
            for (auto const& unit : city.units())
            {
                game::Selection candidate;
                candidate.kind = game::Selection::Kind::Unit;
                candidate.city = city.name();
                candidate.unit = unit.get();
                consider(worldToScreen(unit->position()), candidate);
            }
        }

        if (state.showAgents)
        {
            for (auto const& agent : city.agents())
            {
                game::Selection candidate;
                candidate.kind = game::Selection::Kind::Agent;
                candidate.city = city.name();
                candidate.agentId = agent->id();
                consider(worldToScreen(agent->position()), candidate);
            }
        }

        if (state.showNodes && state.showPaths)
        {
            for (auto const& path : city.paths())
            {
                for (auto const& node : path.second->nodes())
                {
                    game::Selection candidate;
                    candidate.kind = game::Selection::Kind::Node;
                    candidate.city = city.name();
                    candidate.node = node.get();
                    consider(worldToScreen(node->position()), candidate);
                }
            }
        }
    }

    if (best.kind != game::Selection::Kind::None)
        return best;

    if (state.showPaths)
    {
        ImVec2 const world = screenToWorld(screen);
        for (auto& it : simulation.cities())
        {
            float offset = 0.0f;
            Way* way = pickWay(*it.second, world, PICK_RADIUS, offset);
            if (way == nullptr)
                continue;
            game::Selection selected;
            selected.kind = game::Selection::Kind::Way;
            selected.city = it.second->name();
            selected.way = way;
            return selected;
        }
    }

    if (!state.hasHoveredCell)
        return {};

    if (state.showAreas)
    {
        auto const cityIt = simulation.cities().find(state.hoveredCity);
        if (cityIt != simulation.cities().end())
        {
            // Walk backwards: the last painted zone is the one drawn on top.
            Areas const& areas = cityIt->second->areas();
            size_t i = areas.size();
            while (i--)
            {
                if (!areas[i]->contains(state.hoveredU, state.hoveredV))
                    continue;

                game::Selection selected;
                selected.kind = game::Selection::Kind::Area;
                selected.city = state.hoveredCity;
                selected.area = areas[i].get();
                selected.u = state.hoveredU;
                selected.v = state.hoveredV;
                return selected;
            }
        }
    }

    game::Selection cell;
    cell.kind = game::Selection::Kind::Cell;
    cell.city = state.hoveredCity;
    cell.u = state.hoveredU;
    cell.v = state.hoveredV;
    return cell;
}

// ----------------------------------------------------------------------------
void CityViewer::pick(Simulation& simulation,
                      game::DebugState& state,
                      ImVec2 const& screen)
{
    state.selection = pickAt(simulation, state, screen);
}

// ----------------------------------------------------------------------------
void CityViewer::updateHover(Simulation& simulation,
                             game::DebugState& state,
                             bool hovered)
{
    state.hasHoveredCell = false;

    if (!hovered)
        return;

    ImVec2 const world = screenToWorld(ImGui::GetIO().MousePos);

    int32_t u;
    int32_t v;
    simulation.world().world2mapPosition(
        Vector3f(world.x, world.y, 0.0f), u, v);

    for (auto const& it : simulation.cities())
    {
        City const& city = *it.second;
        if (!city.region().contains(u, v))
            continue;

        state.hasHoveredCell = true;
        state.hoveredCity = city.name();
        state.hoveredU = u;
        state.hoveredV = v;
        return;
    }
}

// ----------------------------------------------------------------------------
int32_t CityViewer::cellsPerSquare(float const pixels, MapRegion const& visible)
{
    // Two reasons to draw a square of several cells rather than one rectangle
    // per cell, and the coarser of the two wins.
    //
    // The first is that below a few pixels a cell is not worth a rectangle of
    // its own: nobody can tell one from its neighbour anyway.
    int32_t square = 1;
    while ((square < Map::CHUNK_SIZE) &&
           (float(square) * pixels < MIN_CELL_PIXELS))
    {
        square *= 2;
    }

    // The second is the one that decides the frame rate. A canvas showing a
    // half million cells at three pixels each holds a hundred and sixty
    // thousand of them, and that many rectangles, times the number of layers
    // shown, is what took a city the size of Chicago down to single figure
    // frame rates. So the number of squares on screen is capped, whatever the
    // zoom and whatever the size of the city.
    uint64_t const shown = visible.area();
    while ((square < Map::CHUNK_SIZE) &&
           (shown > MAX_MAP_SQUARES * uint64_t(square) * uint64_t(square)))
    {
        square *= 2;
    }

    return square;
}

// ----------------------------------------------------------------------------
void CityViewer::drawMaps(World& world, game::DebugState const& state)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_MAPS);

    float const side = world.cellSize();
    float const pixels = side * m_zoom;

    // What the canvas shows, in cells, with a margin of one so that the cells
    // straddling the border are still drawn. A city of half a million cells
    // would otherwise cost half a million rectangles a frame whatever the zoom.
    ImVec2 const topLeft = screenToWorld(m_canvas_origin);
    ImVec2 const bottomRight = screenToWorld(m_canvas_origin + m_canvas_size);
    int32_t const uMin = int32_t(std::floor(topLeft.x / side)) - 1;
    int32_t const vMin = int32_t(std::floor(topLeft.y / side)) - 1;
    int32_t const uMax = int32_t(std::floor(bottomRight.x / side)) + 1;
    int32_t const vMax = int32_t(std::floor(bottomRight.y / side)) + 1;
    MapRegion const visible{ uMin,
                             vMin,
                             uint32_t(std::max(0, uMax - uMin)),
                             uint32_t(std::max(0, vMax - vMin)) };

    int32_t const square = cellsPerSquare(pixels, visible);

    for (auto const& it : world.maps())
    {
        Map const& map = *it.second;
        if (!state.isLayerVisible(map.type()))
            continue;

        auto const settings = state.layers.find(map.type());
        game::LayerSettings const options = (settings != state.layers.end())
                                                ? settings->second
                                                : game::LayerSettings();

        bool const primary = (state.primaryLayer == map.type()) ||
                             (state.soloLayer == map.type());
        uint32_t const capacity = std::max(1u, map.getCapacity());

        // Only the cells that hold something are stored, only those that are
        // on screen are worth drawing, and past a point not even all of those:
        // see cellsPerSquare().
        map.forEachBlockInRegion(
            visible,
            square,
            [&](int32_t u, int32_t v, int32_t cells, uint32_t amount)
            {
                float const ratio =
                    std::min(1.0f, float(amount) / float(capacity));
                float const size = pixels * float(cells);

                ImVec2 const p0 =
                    worldToScreen(float(u) * side, float(v) * side);
                ImVec2 const p1(p0.x + size, p0.y + size);

                switch (options.mode)
                {
                    case game::LayerMode::Heatmap:
                    {
                        // A non primary map is drawn thinner so that several of
                        // them stay readable when superimposed.
                        float const inset = primary ? 0.0f : size * 0.18f;
                        // A cell holding a tenth of the capacity would be all
                        // but invisible if the opacity were the ratio itself,
                        // so the ratio shades a floor instead of scaling from
                        // zero: any cell that holds something can be seen. The
                        // floor is low enough for a full map not to bury the
                        // ones under it.
                        float const alpha =
                            options.opacity * (0.15f + 0.85f * ratio);
                        m_draw_list->AddRectFilled(
                            ImVec2(p0.x + inset, p0.y + inset),
                            ImVec2(p1.x - inset, p1.y - inset),
                            theme::fromScript(map.color(), alpha));
                        break;
                    }
                    case game::LayerMode::Contour:
                        m_draw_list->AddRect(
                            p0,
                            p1,
                            theme::fromScript(map.color(), options.opacity),
                            0.0f,
                            0,
                            1.0f + 2.0f * ratio);
                        break;

                    case game::LayerMode::Value:
                    {
                        if (!primary)
                            break;
                        if (size < 22.0f)
                            break;
                        std::string const label = std::to_string(amount);
                        ImVec2 const textSize =
                            ImGui::CalcTextSize(label.c_str());
                        m_draw_list->AddText(
                            ImVec2(0.5f * (p0.x + p1.x) - 0.5f * textSize.x,
                                   0.5f * (p0.y + p1.y) - 0.5f * textSize.y),
                            theme::fromScript(map.color(), options.opacity),
                            label.c_str());
                        break;
                    }
                }
            });
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawCityFrame(City const& city, game::DebugState const& state)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_GRID);

    float const side = city.gridCellSize();
    ImVec2 const topLeft = worldToScreen(city.position());
    ImVec2 const bottomRight =
        worldToScreen(city.position().x + float(city.gridSizeU()) * side,
                      city.position().y + float(city.gridSizeV()) * side);

    if (state.showGrid)
    {
        float const pixels = side * m_zoom;
        // Skip the inner lines when the cells collapse into a gray mush.
        if (pixels >= 4.0f)
        {
            for (uint32_t u = 1u; u < city.gridSizeU(); ++u)
            {
                float const x = topLeft.x + float(u) * pixels;
                m_draw_list->AddLine(ImVec2(x, topLeft.y),
                                     ImVec2(x, bottomRight.y),
                                     theme::GRID_LINE);
            }
            for (uint32_t v = 1u; v < city.gridSizeV(); ++v)
            {
                float const y = topLeft.y + float(v) * pixels;
                m_draw_list->AddLine(ImVec2(topLeft.x, y),
                                     ImVec2(bottomRight.x, y),
                                     theme::GRID_LINE);
            }
        }
    }

    m_draw_list->AddRect(
        topLeft, bottomRight, theme::GRID_LINE_STRONG, 0.0f, 0, 2.0f);

    if (state.showLabels)
    {
        m_draw_list->AddText(ImVec2(topLeft.x + 4.0f, topLeft.y - 18.0f),
                             IM_COL32(200, 205, 215, 220),
                             city.name().c_str());
    }

    // The cell under the cursor is not highlighted here: it belongs to the
    // Inspect tool, which only lights it up when there is nothing more
    // specific under the mouse. See drawInspectHover.
}

// ----------------------------------------------------------------------------
void CityViewer::drawAreas(City& city)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_GRID);

    float const side = city.gridCellSize();
    for (auto const& area : city.areas())
    {
        MapRegion const& region = area->footprint();
        Vector3f const topLeftWorld =
            city.world().mapPosition2world(region.u0, region.v0);
        ImVec2 const p0 = worldToScreen(topLeftWorld);
        ImVec2 const p1 =
            worldToScreen(topLeftWorld.x + float(region.sizeU) * side,
                          topLeftWorld.y + float(region.sizeV) * side);
        ImU32 const fill = theme::fromScript(area->color(), 0.18f);
        ImU32 const line = theme::fromScript(area->color(), 0.70f);
        m_draw_list->AddRectFilled(p0, p1, fill);
        m_draw_list->AddRect(p0, p1, line, 0.0f, 0, 2.0f);
        if (m_zoom > 0.4f)
        {
            m_draw_list->AddText(
                ImVec2(p0.x + 4.0f, p0.y + 2.0f), line, area->type().c_str());
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawPaths(City& city, game::DebugState const& state)
{
    if (!state.showPaths)
        return;

    bool const details = m_zoom > DETAIL_ZOOM_THRESHOLD;

    for (auto const& it : city.paths())
    {
        Path const& path = *it.second;

        m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_WAYS);
        for (auto& way : path.ways())
        {
            ImVec2 const p1 = worldToScreen(way->position1());
            ImVec2 const p2 = worldToScreen(way->position2());

            ImU32 color;
            float thickness;

            if (state.showTraffic)
            {
                // The saturation is what makes the network readable at a
                // glance: a red and fat segment is a jam.
                float const saturation = std::min(1.5f, way->saturation());
                color = theme::congestionColor(std::min(1.0f, saturation));
                thickness = 2.0f + 4.0f * std::min(1.0f, saturation);
            }
            else
            {
                color = theme::fromScript(way->color());
                thickness = 3.0f;
            }

            m_draw_list->AddLine(p1, p2, color, thickness);
        }

        if (!state.showNodes || !details)
            continue;

        m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_NODES);
        for (auto& node : path.nodes())
        {
            ImVec2 const position = worldToScreen(node->position());
            m_draw_list->AddCircleFilled(
                position, NODE_RADIUS, IM_COL32(200, 205, 215, 220));
            m_draw_list->AddCircle(
                position, NODE_RADIUS, IM_COL32(20, 22, 26, 255), 0, 1.5f);
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawUnits(City& city, game::DebugState const& state)
{
    if (!state.showUnits)
        return;

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_UNITS);

    bool const labels = state.showLabels && (m_zoom > LABEL_ZOOM_THRESHOLD);

    for (auto const& unit : city.units())
    {
        ImVec2 const position = worldToScreen(unit->position());
        ImU32 const color = theme::fromScript(unit->color());

        // The driveway: a building stands on its own cell but reaches the
        // network through a Way, and that link is what makes it part of the
        // city rather than a house in a field.
        Way const* const way = unit->way();
        if ((way != nullptr) && state.showPaths)
        {
            Vector3f const anchor = way->positionAt(unit->wayOffset());
            ImVec2 const onRoad = worldToScreen(anchor);
            float const dx = onRoad.x - position.x;
            float const dy = onRoad.y - position.y;
            if ((dx * dx + dy * dy) > (UNIT_RADIUS * UNIT_RADIUS))
            {
                m_draw_list->AddLine(position,
                                     onRoad,
                                     theme::fromScript(unit->color(), 0.45f),
                                     1.5f);
            }
        }

        // A square, so that a Unit is never confused with a Node or an Agent.
        m_draw_list->AddRectFilled(
            ImVec2(position.x - UNIT_RADIUS, position.y - UNIT_RADIUS),
            ImVec2(position.x + UNIT_RADIUS, position.y + UNIT_RADIUS),
            color,
            2.0f);
        m_draw_list->AddRect(
            ImVec2(position.x - UNIT_RADIUS, position.y - UNIT_RADIUS),
            ImVec2(position.x + UNIT_RADIUS, position.y + UNIT_RADIUS),
            IM_COL32(15, 16, 20, 255),
            2.0f,
            0,
            1.5f);

        if (labels)
        {
            m_draw_list->AddText(
                ImVec2(position.x + UNIT_RADIUS + 3.0f, position.y - 7.0f),
                IM_COL32(220, 225, 235, 220),
                unit->type().c_str());
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawAgents(City& city, game::DebugState const& state)
{
    if (!state.showAgents)
        return;

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_AGENTS);

    for (auto const& agent : city.agents())
    {
        ImVec2 const position = worldToScreen(agent->position());
        m_draw_list->AddCircleFilled(
            position, AGENT_RADIUS, theme::fromScript(agent->color()));
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawInspectHover(Simulation& simulation,
                                  game::DebugState const& state,
                                  editor::Editor const& /*editor*/)
{
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        return;

    game::Selection const hover =
        pickAt(simulation, state, ImGui::GetIO().MousePos);
    if (hover.kind == game::Selection::Kind::None)
        return;

    ImU32 const highlight = theme::fromScript(0x56AADE, 0.85f);

    switch (hover.kind)
    {
        case game::Selection::Kind::Cell:
        {
            auto const cityIt = simulation.cities().find(hover.city);
            if (cityIt == simulation.cities().end())
                break;
            City& city = *cityIt->second;
            float const side = city.gridCellSize();
            ImVec2 const p0 =
                worldToScreen(city.world().mapPosition2world(hover.u, hover.v));
            ImVec2 const p1(p0.x + side * m_zoom, p0.y + side * m_zoom);
            m_draw_list->AddRectFilled(
                p0, p1, theme::fromScript(0x56AADE, 0.18f));
            m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 2.5f);
            break;
        }
        case game::Selection::Kind::Node:
        {
            if (hover.node == nullptr)
                break;
            m_draw_list->AddCircle(worldToScreen(hover.node->position()),
                                   NODE_RADIUS + 8.0f,
                                   highlight,
                                   0,
                                   2.5f);
            break;
        }
        case game::Selection::Kind::Way:
        {
            if (hover.way == nullptr)
                break;
            ImVec2 const a = worldToScreen(hover.way->position1());
            ImVec2 const b = worldToScreen(hover.way->position2());
            m_draw_list->AddLine(a, b, highlight, 6.0f);
            break;
        }
        case game::Selection::Kind::Unit:
        {
            if (hover.unit == nullptr)
                break;
            m_draw_list->AddCircle(worldToScreen(hover.unit->position()),
                                   UNIT_RADIUS + 8.0f,
                                   highlight,
                                   0,
                                   2.5f);
            break;
        }
        case game::Selection::Kind::Agent:
        {
            Agent const* agent = hover.resolveAgent(simulation);
            if (agent == nullptr)
                break;
            m_draw_list->AddCircle(worldToScreen(agent->position()),
                                   AGENT_RADIUS + 8.0f,
                                   highlight,
                                   0,
                                   2.5f);
            break;
        }
        case game::Selection::Kind::Area:
        {
            auto const cityIt = simulation.cities().find(hover.city);
            if ((hover.area == nullptr) ||
                (cityIt == simulation.cities().end()))
                break;
            City& city = *cityIt->second;
            float const side = city.gridCellSize();
            MapRegion const& footprint = hover.area->footprint();
            ImVec2 const p0 = worldToScreen(
                city.world().mapPosition2world(footprint.u0, footprint.v0));
            ImVec2 const p1(p0.x + float(footprint.sizeU) * side * m_zoom,
                            p0.y + float(footprint.sizeV) * side * m_zoom);
            m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 2.5f);
            break;
        }
        case game::Selection::Kind::None:
            break;
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawSelectionOverlay(Simulation& simulation,
                                      game::DebugState const& state,
                                      editor::Editor const& editor)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_OVERLAY);

    if (editor.tool() == editor::EditTool::Select)
    {
        drawInspectHover(simulation, state, editor);
    }

    game::Selection const& selection = state.selection;
    if (selection.kind == game::Selection::Kind::None)
        return;

    auto const cityIt = simulation.cities().find(selection.city);
    if (cityIt == simulation.cities().end())
        return;
    City& city = *cityIt->second;

    ImU32 const highlight = theme::ACCENT;

    switch (selection.kind)
    {
        case game::Selection::Kind::Unit:
        {
            if (selection.unit == nullptr)
                break;

            ImVec2 const position = worldToScreen(selection.unit->position());
            m_draw_list->AddCircle(
                position, UNIT_RADIUS + 6.0f, highlight, 0, 2.0f);

            if (state.showSelectionRadius)
            {
                // Materialize the disc the rules of this Unit read and write on
                // the maps. Seeing it is the quickest way to understand why a
                // Unit does not reach the resource next door.
                float const radius = float(selection.unit->mapRadius()) *
                                     city.gridCellSize() * m_zoom;
                if (radius > 1.0f)
                {
                    m_draw_list->AddCircle(position,
                                           radius,
                                           theme::fromScript(0x56AADE, 0.7f),
                                           0,
                                           1.5f);
                }
            }
            break;
        }
        case game::Selection::Kind::Node:
        {
            if (selection.node == nullptr)
                break;
            m_draw_list->AddCircle(worldToScreen(selection.node->position()),
                                   NODE_RADIUS + 6.0f,
                                   highlight,
                                   0,
                                   2.0f);
            break;
        }
        case game::Selection::Kind::Agent:
        {
            Agent const* const agent = selection.resolveAgent(simulation);
            if (agent == nullptr)
                break;

            ImVec2 const position = worldToScreen(agent->position());
            m_draw_list->AddCircle(
                position, AGENT_RADIUS + 6.0f, highlight, 0, 2.0f);

            Route const& route = agent->route();
            if (route.found)
            {
                ImVec2 previous = position;
                for (Node const* node : route)
                {
                    ImVec2 const next = worldToScreen(node->position());
                    m_draw_list->AddLine(previous, next, highlight, 2.0f);
                    previous = next;
                }
                if (route.destination != nullptr)
                {
                    m_draw_list->AddLine(
                        previous,
                        worldToScreen(route.destination->position()),
                        highlight,
                        2.0f);
                }
            }
            else
            {
                for (auto const& unit : city.units())
                {
                    if (unit->type() != agent->searchTarget())
                        continue;

                    m_draw_list->AddLine(position,
                                         worldToScreen(unit->position()),
                                         theme::fromScript(0x56AADE, 0.45f),
                                         1.5f);
                }
            }
            break;
        }
        case game::Selection::Kind::Cell:
        {
            float const side = city.gridCellSize();
            ImVec2 const p0 = worldToScreen(
                city.world().mapPosition2world(selection.u, selection.v));
            ImVec2 const p1(p0.x + side * m_zoom, p0.y + side * m_zoom);
            m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 2.5f);
            break;
        }
        case game::Selection::Kind::Area:
        {
            if (selection.area == nullptr)
                break;
            float const side = city.gridCellSize();
            MapRegion const& footprint = selection.area->footprint();
            ImVec2 const p0 = worldToScreen(
                city.world().mapPosition2world(footprint.u0, footprint.v0));
            ImVec2 const p1(p0.x + float(footprint.sizeU) * side * m_zoom,
                            p0.y + float(footprint.sizeV) * side * m_zoom);
            m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 3.0f);
            break;
        }
        case game::Selection::Kind::Way:
        {
            if (selection.way == nullptr)
                break;
            ImVec2 const a = worldToScreen(selection.way->position1());
            ImVec2 const b = worldToScreen(selection.way->position2());
            m_draw_list->AddLine(a, b, highlight, 5.0f);
            break;
        }
        case game::Selection::Kind::None:
            break;
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawLegend(game::DebugState const& state)
{
    if (!state.showTraffic)
        return;

    // Anchored to the bottom right corner: the bottom left one already holds
    // the hint of the armed tool.
    float const width = 140.0f;
    float const height = 10.0f;
    ImVec2 const origin(m_canvas_origin.x + m_canvas_size.x - width - 12.0f,
                        m_canvas_origin.y + m_canvas_size.y - 34.0f);

    for (int i = 0; i < 32; ++i)
    {
        float const t0 = float(i) / 32.0f;
        float const t1 = float(i + 1) / 32.0f;
        m_draw_list->AddRectFilled(
            ImVec2(origin.x + t0 * width, origin.y),
            ImVec2(origin.x + t1 * width, origin.y + height),
            theme::congestionColor(t0));
    }

    m_draw_list->AddText(
        ImVec2(origin.x, origin.y - 16.0f), theme::MUTED, "free");
    m_draw_list->AddText(ImVec2(origin.x + width - 44.0f, origin.y - 16.0f),
                         theme::MUTED,
                         "jammed");
}

// ----------------------------------------------------------------------------
void CityViewer::drawDisplayToggles(game::DebugState& state) const
{
    struct Toggle
    {
        char const* label;
        bool* value;
    };

    std::array<Toggle, 7> const toggles = { {
        { "Grid", &state.showGrid },
        { "Paths", &state.showPaths },
        { "Units", &state.showUnits },
        { "Areas", &state.showAreas },
        { "Agents", &state.showAgents },
        { "Traffic", &state.showTraffic },
        { "Labels", &state.showLabels },
    } };

    float const right =
        ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    for (size_t i = 0u; i < toggles.size(); ++i)
    {
        float const width = ImGui::CalcTextSize(toggles[i].label).x +
                            ImGui::GetFrameHeight() +
                            2.0f * ImGui::GetStyle().FramePadding.x +
                            ImGui::GetStyle().ItemInnerSpacing.x;
        float const next =
            ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width;
        if (next < right)
            ImGui::SameLine();
        ImGui::Checkbox(toggles[i].label, toggles[i].value);
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawHoverTooltip(Simulation& simulation,
                                  game::DebugState const& state,
                                  editor::Editor const& editor)
{
    if (editor.tool() != editor::EditTool::Select)
        return;
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        return;

    game::Selection const hover =
        pickAt(simulation, state, ImGui::GetIO().MousePos);
    if (hover.kind == game::Selection::Kind::None)
        return;

    ImGui::BeginTooltip();

    switch (hover.kind)
    {
        case game::Selection::Kind::Unit:
            if (hover.unit != nullptr)
            {
                Unit const& unit = *hover.unit;
                uint32_t const hour = simulation.clock().hourOfDay();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                       theme::fromScript(unit.color())),
                                   "%s #%u",
                                   unit.type().c_str(),
                                   unit.id());

                // A building doing nothing at three in the morning is shut, not
                // broken, and the timetable of its rules is the only thing that
                // says which.
                OpeningStatus const opening = openingStatus(unit, hour);
                ImGui::TextColored(
                    ImGui::ColorConvertU32ToFloat4(
                        opening.open ? theme::SUCCESS : theme::FAILURE),
                    "%s",
                    opening.text.c_str());

                // What a building holds and what it tries to do is the whole
                // reason to point at it. Without them the tooltip only repeated
                // the label already drawn on the map.
                ImGui::Separator();
                std::vector<Resource> const& bin = unit.resources().container();
                if (bin.empty())
                {
                    ImGui::TextDisabled("holds nothing");
                }
                else
                {
                    for (Resource const& resource : bin)
                    {
                        ImGui::Text("%s %u / %u",
                                    resource.type().c_str(),
                                    resource.getAmount(),
                                    resource.getCapacity());
                    }
                }

                ImGui::Separator();
                if (unit.rules().empty())
                {
                    ImGui::TextDisabled("no rule");
                }
                else
                {
                    for (RuleUnit const* rule : unit.rules())
                    {
                        if (rule == nullptr)
                            continue;

                        OpeningHours hours;
                        hours.add(*rule);
                        if (hours.bounded() && !hours.isOpen(hour))
                        {
                            uint32_t const next = hours.nextOpening(hour);
                            if (next == OpeningHours::NEVER)
                            {
                                ImGui::BulletText("%s (inactive)",
                                                  rule->type().c_str());
                            }
                            else
                            {
                                ImGui::BulletText("%s (inactive until %uh)",
                                                  rule->type().c_str(),
                                                  next);
                            }
                        }
                        else
                        {
                            ImGui::BulletText("%s (active)",
                                              rule->type().c_str());
                        }
                    }
                }

                if (!unit.hasWays())
                {
                    ImGui::TextColored(
                        ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                        "no road: unreachable");
                }
            }
            break;
        case game::Selection::Kind::Agent:
        {
            Agent const* agent = hover.resolveAgent(simulation);
            if (agent != nullptr)
            {
                ImGui::Text("Agent %s #%u", agent->type().c_str(), agent->id());
                ImGui::TextDisabled("looking for %s",
                                    agent->searchTarget().c_str());
            }
            break;
        }
        case game::Selection::Kind::Node:
            if (hover.node != nullptr)
            {
                ImGui::Text("Node #%u", hover.node->id());
                if (hover.node->ways().empty())
                    ImGui::TextDisabled("orphan");
                for (Way const* way : hover.node->ways())
                {
                    ImGui::BulletText(
                        "%s, %.1f m", way->type().c_str(), way->magnitude());
                }

                for (Unit const* unit : hover.node->units())
                {
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                           theme::fromScript(unit->color())),
                                       "%s #%u stands here",
                                       unit->type().c_str(),
                                       unit->id());
                }
            }
            break;
        case game::Selection::Kind::Way:
            if (hover.way != nullptr)
            {
                Way const& way = *hover.way;
                ImGui::Text("Way %s #%u", way.type().c_str(), way.id());
                ImGui::Text("%.1f m", way.magnitude());
                ImGui::Text("free flow %.2f s, now %.2f s",
                            way.freeFlowTime(),
                            way.travelTime());
                float const saturation = way.saturation();
                char overlay[64];
                std::snprintf(overlay,
                              sizeof(overlay),
                              "%.0f / %.0f agents",
                              way.flow(),
                              way.capacity());
                ImGui::Text("saturation");
                ImGui::SameLine(120.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                                      ImGui::ColorConvertU32ToFloat4(
                                          theme::congestionColor(saturation)));
                ImGui::ProgressBar(
                    std::min(1.0f, saturation), ImVec2(140.0f, 0.0f), overlay);
                ImGui::PopStyleColor();
            }
            break;
        case game::Selection::Kind::Area:
        {
            if (hover.area == nullptr)
                break;
            MapRegion const& footprint = hover.area->footprint();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                   theme::fromScript(hover.area->color())),
                               "Zone %s #%u",
                               hover.area->type().c_str(),
                               hover.area->id());
            ImGui::TextDisabled(
                "%u x %u cells", footprint.sizeU, footprint.sizeV);
            ImGui::Text("%zu building(s) inside",
                        hover.area->unitsInside().size());
            for (RuleArea const* rule : hover.area->rules())
            {
                if (rule != nullptr)
                    ImGui::BulletText("%s", rule->type().c_str());
            }
            break;
        }
        case game::Selection::Kind::Cell:
        {
            auto const it = simulation.cities().find(hover.city);
            if (it == simulation.cities().end())
                break;
            City& city = *it->second;
            ImGui::TextUnformatted(city.name().c_str());
            ImGui::Separator();
            ImGui::Text("cell (%d, %d)", hover.u, hover.v);
            if (city.maps().empty())
            {
                ImGui::TextDisabled("no map");
            }
            else
            {
                for (auto const& mapIt : city.maps())
                {
                    Map const& map = *mapIt.second;
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                           theme::fromScript(map.color())),
                                       "%s",
                                       map.type().c_str());
                    ImGui::SameLine();
                    ImGui::Text(": %u / %u",
                                map.getResource(hover.u, hover.v),
                                map.getCapacity());
                }
            }
            break;
        }
        case game::Selection::Kind::None:
            break;
    }

    ImGui::EndTooltip();
}

// ----------------------------------------------------------------------------
void CityViewer::drawClockHud(Simulation const& simulation)
{
    SimulationClock const& clock = simulation.clock();
    float const hour =
        float(clock.hourOfDay()) + float(clock.minuteOfHour()) / 60.0f;

    char time[16];
    std::snprintf(time,
                  sizeof(time),
                  "%02u:%02u",
                  clock.hourOfDay(),
                  clock.minuteOfHour());
    char day[24];
    std::snprintf(day, sizeof(day), "Jour %u", clock.day());

    ImU32 const color = theme::clockHudColor(hour);
    ImVec2 const pos(m_canvas_origin.x + 12.0f, m_canvas_origin.y + 10.0f);
    m_draw_list->AddText(
        ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 180), time);
    m_draw_list->AddText(pos, color, time);

    ImVec2 const dayPos(pos.x, pos.y + 16.0f);
    m_draw_list->AddText(
        ImVec2(dayPos.x + 1.0f, dayPos.y + 1.0f), IM_COL32(0, 0, 0, 160), day);
    m_draw_list->AddText(dayPos, theme::MUTED, day);
}

// ----------------------------------------------------------------------------
void CityViewer::drawHint(editor::Editor const& editor)
{
    std::string const hint = editor.hint();
    if (hint.empty())
        return;

    ImVec2 const size = ImGui::CalcTextSize(hint.c_str());
    ImVec2 const pos(m_canvas_origin.x + 12.0f,
                     m_canvas_origin.y + m_canvas_size.y - size.y - 14.0f);
    m_draw_list->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f),
                         IM_COL32(0, 0, 0, 180),
                         hint.c_str());
    m_draw_list->AddText(pos, theme::ACCENT, hint.c_str());
}
} // namespace ui
} // namespace ogb
