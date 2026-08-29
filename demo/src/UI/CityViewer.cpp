//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/CityViewer.hpp"
#include "Editor/Editor.hpp"
#include "OpenGlassBox/Config.hpp"
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
//! own, and the layers are drawn by squares of several cells instead.
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

    for (auto const& it : simulation.getCities())
    {
        City const& city = *it.second;

        // The grid of the city always bounds its layers, and the nodes may stick
        // out of it, so take the union of both.
        float const side = city.getCellSize();
        minX = std::min(minX, city.getPosition().x);
        minY = std::min(minY, city.getPosition().y);
        maxX =
            std::max(maxX, city.getPosition().x + float(city.getRegion().sizeU) * side);
        maxY =
            std::max(maxY, city.getPosition().y + float(city.getRegion().sizeV) * side);
        found = true;

        for (auto const& path : city.getPaths())
        {
            for (auto const& node : path.second->getNodes())
            {
                minX = std::min(minX, node->getPosition().x);
                minY = std::min(minY, node->getPosition().y);
                maxX = std::max(maxX, node->getPosition().x);
                maxY = std::max(maxY, node->getPosition().y);
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
    if (!ImGui::Begin("Layer"))
    {
        ImGui::End();
        return;
    }

    drawToolbar(simulation, state, editor);

    SimulationClock const& clock = simulation.getClock();
    float const hour =
        float(clock.getHourOfDay()) + float(clock.getMinuteOfHour()) / 60.0f;
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

    // The layers belong to the world and span every city, so they are drawn once
    // rather than once per city.
    drawLayers(simulation, state);

    for (auto& it : simulation.getCities())
    {
        City& city = *it.second;
        drawCityFrame(city, state);
        if (state.showZones)
            drawZones(city);
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
    // The child window is not a widget, so claim the zone explicitly to know
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
Segment* CityViewer::pickSegment(City& city,
                         ImVec2 const& world,
                         float pixels,
                         float& offset) const
{
    // Work in world units so that the tolerance stays the same on screen
    // whatever the zoom.
    float const tolerance = pixels / std::max(1e-3f, m_zoom);
    float best = tolerance * tolerance;
    Segment* found = nullptr;

    for (auto const& it : city.getPaths())
    {
        for (auto& segment : it.second->getSegments())
        {
            float candidate = 0.0f;
            float const distance = distanceToSegment(
                world, segment->getFromPosition(), segment->getToPosition(), candidate);
            if (distance <= best)
            {
                best = distance;
                found = segment.get();
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

    for (auto const& it : city.getPaths())
    {
        for (auto& node : it.second->getNodes())
        {
            float const dx = node->getPosition().x - world.x;
            float const dy = node->getPosition().y - world.y;
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

    for (auto const& unit : city.getUnits())
    {
        float const dx = unit->getPosition().x - world.x;
        float const dy = unit->getPosition().y - world.y;
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

    for (auto const& agent : city.getAgents())
    {
        float const dx = agent->getPosition().x - world.x;
        float const dy = agent->getPosition().y - world.y;
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
    float bestDistance = ROUTING_INFINITY;
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

    for (auto& it : simulation.getCities())
    {
        City& city = *it.second;

        if (state.showUnits)
        {
            for (auto const& unit : city.getUnits())
            {
                game::Selection candidate;
                candidate.kind = game::Selection::Kind::Unit;
                candidate.city = city.getName();
                candidate.unit = unit.get();
                consider(worldToScreen(unit->getPosition()), candidate);
            }
        }

        if (state.showAgents)
        {
            for (auto const& agent : city.getAgents())
            {
                game::Selection candidate;
                candidate.kind = game::Selection::Kind::Agent;
                candidate.city = city.getName();
                candidate.agentId = agent->getId();
                consider(worldToScreen(agent->getPosition()), candidate);
            }
        }

        if (state.showNodes && state.showPaths)
        {
            for (auto const& path : city.getPaths())
            {
                for (auto const& node : path.second->getNodes())
                {
                    game::Selection candidate;
                    candidate.kind = game::Selection::Kind::Node;
                    candidate.city = city.getName();
                    candidate.node = node.get();
                    consider(worldToScreen(node->getPosition()), candidate);
                }
            }
        }
    }

    if (best.kind != game::Selection::Kind::None)
        return best;

    if (state.showPaths)
    {
        ImVec2 const world = screenToWorld(screen);
        for (auto& it : simulation.getCities())
        {
            float offset = 0.0f;
            Segment* segment = pickSegment(*it.second, world, PICK_RADIUS, offset);
            if (segment == nullptr)
                continue;
            game::Selection selected;
            selected.kind = game::Selection::Kind::Segment;
            selected.city = it.second->getName();
            selected.segment = segment;
            return selected;
        }
    }

    if (!state.hasHoveredCell)
        return {};

    if (state.showZones)
    {
        auto const cityIt = simulation.getCities().find(state.hoveredCity);
        if (cityIt != simulation.getCities().end())
        {
            // Walk backwards: the last painted zone is the one drawn on top.
            Zones const& zones = cityIt->second->getZones();
            size_t i = zones.size();
            while (i--)
            {
                if (!zones[i]->contains({ state.hoveredU, state.hoveredV }))
                    continue;

                game::Selection selected;
                selected.kind = game::Selection::Kind::Zone;
                selected.city = state.hoveredCity;
                selected.zone = zones[i].get();
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

    Cell const cell = simulation.worldToCell({ world.x, world.y, 0.0f });

    for (auto const& it : simulation.getCities())
    {
        City const& city = *it.second;
        if (!city.getRegion().contains(cell))
            continue;

        state.hasHoveredCell = true;
        state.hoveredCity = city.getName();
        state.hoveredU = cell.u;
        state.hoveredV = cell.v;
        return;
    }
}

// ----------------------------------------------------------------------------
int32_t CityViewer::cellsPerSquare(float const pixels, CellRegion const& visible)
{
    // Two reasons to draw a square of several cells rather than one rectangle
    // per cell, and the coarser of the two wins.
    //
    // The first is that below a few pixels a cell is not worth a rectangle of
    // its own: nobody can tell one from its neighbour anyway.
    int32_t square = 1;
    while ((square < Layer::CHUNK_SIZE) &&
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
    uint64_t const shown = visible.getCellCount();
    while ((square < Layer::CHUNK_SIZE) &&
           (shown > MAX_MAP_SQUARES * uint64_t(square) * uint64_t(square)))
    {
        square *= 2;
    }

    return square;
}

// ----------------------------------------------------------------------------
void CityViewer::drawLayers(Simulation& simulation,
                          game::DebugState const& state)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_MAPS);

    float const side = simulation.getConfig().grid.cellSize;
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
    CellRegion const visible{ uMin,
                             vMin,
                             uint32_t(std::max(0, uMax - uMin)),
                             uint32_t(std::max(0, vMax - vMin)) };

    int32_t const square = cellsPerSquare(pixels, visible);

    for (auto const& it : simulation.getLayers())
    {
        Layer const& layer = *it.second;
        if (!state.isLayerVisible(layer.getTypeName().str()))
            continue;

        auto const settings = state.layers.find(layer.getTypeName().str());
        game::LayerSettings const options = (settings != state.layers.end())
                                                ? settings->second
                                                : game::LayerSettings();

        bool const primary = (state.primaryLayer == layer.getTypeName().str()) ||
                             (state.soloLayer == layer.getTypeName().str());
        uint32_t const capacity = std::max(1u, layer.getCellCapacity());

        // Only the cells that hold something are stored, only those that are
        // on screen are worth drawing, and past a point not even all of those:
        // see cellsPerSquare().
        layer.forEachBlockInRegion(
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
                        // A non primary layer is drawn thinner so that several of
                        // them stay readable when superimposed.
                        float const inset = primary ? 0.0f : size * 0.18f;
                        // A cell holding a tenth of the capacity would be all
                        // but invisible if the opacity were the ratio itself,
                        // so the ratio shades a floor instead of scaling from
                        // zero: any cell that holds something can be seen. The
                        // floor is low enough for a full layer not to bury the
                        // ones under it.
                        float const alpha =
                            options.opacity * (0.15f + 0.85f * ratio);
                        m_draw_list->AddRectFilled(
                            ImVec2(p0.x + inset, p0.y + inset),
                            ImVec2(p1.x - inset, p1.y - inset),
                            theme::fromScript(layer.getColor(), alpha));
                        break;
                    }
                    case game::LayerMode::Contour:
                        m_draw_list->AddRect(
                            p0,
                            p1,
                            theme::fromScript(layer.getColor(), options.opacity),
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
                            theme::fromScript(layer.getColor(), options.opacity),
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

    float const side = city.getCellSize();
    ImVec2 const topLeft = worldToScreen(city.getPosition());
    ImVec2 const bottomRight =
        worldToScreen(city.getPosition().x + float(city.getRegion().sizeU) * side,
                      city.getPosition().y + float(city.getRegion().sizeV) * side);

    if (state.showGrid)
    {
        float const pixels = side * m_zoom;
        // Skip the inner lines when the cells collapse into a gray mush.
        if (pixels >= 4.0f)
        {
            for (uint32_t u = 1u; u < city.getRegion().sizeU; ++u)
            {
                float const x = topLeft.x + float(u) * pixels;
                m_draw_list->AddLine(ImVec2(x, topLeft.y),
                                     ImVec2(x, bottomRight.y),
                                     theme::GRID_LINE);
            }
            for (uint32_t v = 1u; v < city.getRegion().sizeV; ++v)
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
                             city.getName().c_str());
    }

    // The cell under the cursor is not highlighted here: it belongs to the
    // Inspect tool, which only lights it up when there is nothing more
    // specific under the mouse. See drawInspectHover.
}

// ----------------------------------------------------------------------------
void CityViewer::drawZones(City& city)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_GRID);

    float const side = city.getCellSize();
    for (auto const& zone : city.getZones())
    {
        CellRegion const& region = zone->getRegion();
        Vector3f const topLeftWorld =
            city.cellToWorld({ region.u0, region.v0 });
        ImVec2 const p0 = worldToScreen(topLeftWorld);
        ImVec2 const p1 =
            worldToScreen(topLeftWorld.x + float(region.sizeU) * side,
                          topLeftWorld.y + float(region.sizeV) * side);
        ImU32 const fill = theme::fromScript(zone->getColor(), 0.18f);
        ImU32 const line = theme::fromScript(zone->getColor(), 0.70f);
        m_draw_list->AddRectFilled(p0, p1, fill);
        m_draw_list->AddRect(p0, p1, line, 0.0f, 0, 2.0f);
        if (m_zoom > 0.4f)
        {
            m_draw_list->AddText(
                ImVec2(p0.x + 4.0f, p0.y + 2.0f), line, zone->getTypeName().c_str());
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawPaths(City& city, game::DebugState const& state)
{
    if (!state.showPaths)
        return;

    bool const details = m_zoom > DETAIL_ZOOM_THRESHOLD;

    for (auto const& it : city.getPaths())
    {
        Path const& path = *it.second;

        m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_WAYS);
        for (auto& segment : path.getSegments())
        {
            ImVec2 const p1 = worldToScreen(segment->getFromPosition());
            ImVec2 const p2 = worldToScreen(segment->getToPosition());

            ImU32 color;
            float thickness;

            if (state.showTraffic)
            {
                // The saturation is what makes the network readable at a
                // glance: a red and fat segment is a jam.
                float const saturation = std::min(1.5f, segment->getSaturation());
                color = theme::congestionColor(std::min(1.0f, saturation));
                thickness = 2.0f + 4.0f * std::min(1.0f, saturation);
            }
            else
            {
                color = theme::fromScript(segment->getColor());
                thickness = 3.0f;
            }

            m_draw_list->AddLine(p1, p2, color, thickness);
        }

        if (!state.showNodes || !details)
            continue;

        m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_NODES);
        for (auto& node : path.getNodes())
        {
            ImVec2 const position = worldToScreen(node->getPosition());
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

    for (auto const& unit : city.getUnits())
    {
        ImVec2 const position = worldToScreen(unit->getPosition());
        ImU32 const color = theme::fromScript(unit->getColor());

        // The driveway: a building stands on its own cell but reaches the
        // network through a Segment, and that link is what makes it part of the
        // city rather than a house in a field.
        Segment const* const segment = unit->getSegment();
        if ((segment != nullptr) && state.showPaths)
        {
            Vector3f const anchor = segment->getPositionAt(unit->getSegmentOffset());
            ImVec2 const onRoad = worldToScreen(anchor);
            float const dx = onRoad.x - position.x;
            float const dy = onRoad.y - position.y;
            if ((dx * dx + dy * dy) > (UNIT_RADIUS * UNIT_RADIUS))
            {
                m_draw_list->AddLine(position,
                                     onRoad,
                                     theme::fromScript(unit->getColor(), 0.45f),
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
                unit->getTypeName().c_str());
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawAgents(City& city, game::DebugState const& state)
{
    if (!state.showAgents)
        return;

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_AGENTS);

    for (auto const& agent : city.getAgents())
    {
        ImVec2 const position = worldToScreen(agent->getPosition());
        m_draw_list->AddCircleFilled(
            position, AGENT_RADIUS, theme::fromScript(agent->getColor()));
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
            auto const cityIt = simulation.getCities().find(hover.city);
            if (cityIt == simulation.getCities().end())
                break;
            City& city = *cityIt->second;
            float const side = city.getCellSize();
            ImVec2 const p0 =
                worldToScreen(city.cellToWorld({ hover.u, hover.v }));
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
            m_draw_list->AddCircle(worldToScreen(hover.node->getPosition()),
                                   NODE_RADIUS + 8.0f,
                                   highlight,
                                   0,
                                   2.5f);
            break;
        }
        case game::Selection::Kind::Segment:
        {
            if (hover.segment == nullptr)
                break;
            ImVec2 const a = worldToScreen(hover.segment->getFromPosition());
            ImVec2 const b = worldToScreen(hover.segment->getToPosition());
            m_draw_list->AddLine(a, b, highlight, 6.0f);
            break;
        }
        case game::Selection::Kind::Unit:
        {
            if (hover.unit == nullptr)
                break;
            m_draw_list->AddCircle(worldToScreen(hover.unit->getPosition()),
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
            m_draw_list->AddCircle(worldToScreen(agent->getPosition()),
                                   AGENT_RADIUS + 8.0f,
                                   highlight,
                                   0,
                                   2.5f);
            break;
        }
        case game::Selection::Kind::Zone:
        {
            auto const cityIt = simulation.getCities().find(hover.city);
            if ((hover.zone == nullptr) ||
                (cityIt == simulation.getCities().end()))
                break;
            City& city = *cityIt->second;
            float const side = city.getCellSize();
            CellRegion const& footprint = hover.zone->getRegion();
            ImVec2 const p0 = worldToScreen(
                city.cellToWorld({ footprint.u0, footprint.v0 }));
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

    auto const cityIt = simulation.getCities().find(selection.city);
    if (cityIt == simulation.getCities().end())
        return;
    City& city = *cityIt->second;

    ImU32 const highlight = theme::ACCENT;

    switch (selection.kind)
    {
        case game::Selection::Kind::Unit:
        {
            if (selection.unit == nullptr)
                break;

            ImVec2 const position = worldToScreen(selection.unit->getPosition());
            m_draw_list->AddCircle(
                position, UNIT_RADIUS + 6.0f, highlight, 0, 2.0f);

            if (state.showSelectionRadius)
            {
                // Materialize the disc the rules of this Unit read and write on
                // the layers. Seeing it is the quickest way to understand why a
                // Unit does not reach the resource next door.
                float const radius = float(selection.unit->getLayerRadius()) *
                                     city.getCellSize() * m_zoom;
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
            m_draw_list->AddCircle(worldToScreen(selection.node->getPosition()),
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

            ImVec2 const position = worldToScreen(agent->getPosition());
            m_draw_list->AddCircle(
                position, AGENT_RADIUS + 6.0f, highlight, 0, 2.0f);

            Route const& route = agent->getRoute();
            if (route.isFound())
            {
                ImVec2 previous = position;
                for (Node const* node : route)
                {
                    ImVec2 const next = worldToScreen(node->getPosition());
                    m_draw_list->AddLine(previous, next, highlight, 2.0f);
                    previous = next;
                }
                if (route.getDestination() != nullptr)
                {
                    m_draw_list->AddLine(
                        previous,
                        worldToScreen(route.getDestination()->getPosition()),
                        highlight,
                        2.0f);
                }
            }
            else
            {
                for (auto const& unit : city.getUnits())
                {
                    if (unit->getTypeName() != agent->getTarget())
                        continue;

                    m_draw_list->AddLine(position,
                                         worldToScreen(unit->getPosition()),
                                         theme::fromScript(0x56AADE, 0.45f),
                                         1.5f);
                }
            }
            break;
        }
        case game::Selection::Kind::Cell:
        {
            float const side = city.getCellSize();
            ImVec2 const p0 = worldToScreen(
                city.cellToWorld({ selection.u, selection.v }));
            ImVec2 const p1(p0.x + side * m_zoom, p0.y + side * m_zoom);
            m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 2.5f);
            break;
        }
        case game::Selection::Kind::Zone:
        {
            if (selection.zone == nullptr)
                break;
            float const side = city.getCellSize();
            CellRegion const& footprint = selection.zone->getRegion();
            ImVec2 const p0 = worldToScreen(
                city.cellToWorld({ footprint.u0, footprint.v0 }));
            ImVec2 const p1(p0.x + float(footprint.sizeU) * side * m_zoom,
                            p0.y + float(footprint.sizeV) * side * m_zoom);
            m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 3.0f);
            break;
        }
        case game::Selection::Kind::Segment:
        {
            if (selection.segment == nullptr)
                break;
            ImVec2 const a = worldToScreen(selection.segment->getFromPosition());
            ImVec2 const b = worldToScreen(selection.segment->getToPosition());
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
        { "Zones", &state.showZones },
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
                uint32_t const hour = simulation.getClock().getHourOfDay();
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                       theme::fromScript(unit.getColor())),
                                   "%s #%u",
                                   unit.getTypeName().c_str(),
                                   unit.getId());

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
                // the label already drawn on the grid.
                ImGui::Separator();
                std::vector<Resource> const& bin = unit.getResources().getAll();
                if (bin.empty())
                {
                    ImGui::TextDisabled("holds nothing");
                }
                else
                {
                    for (Resource const& resource : bin)
                    {
                        ImGui::Text("%s %u / %u",
                                    resource.getTypeName().c_str(),
                                    resource.getAmount(),
                                    resource.getCapacity());
                    }
                }

                ImGui::Separator();
                if (unit.getRules().empty())
                {
                    ImGui::TextDisabled("no rule");
                }
                else
                {
                    for (RuleUnit const* rule : unit.getRules())
                    {
                        if (rule == nullptr)
                            continue;

                        OpeningHours hours;
                        hours.add(*rule);
                        if (hours.isRestricted() && !hours.isOpen(hour))
                        {
                            uint32_t const next = hours.getNextOpeningHour(hour);
                            if (next == OpeningHours::NEVER)
                            {
                                ImGui::BulletText("%s (inactive)",
                                                  rule->getName().c_str());
                            }
                            else
                            {
                                ImGui::BulletText("%s (inactive until %uh)",
                                                  rule->getName().c_str(),
                                                  next);
                            }
                        }
                        else
                        {
                            ImGui::BulletText("%s (active)",
                                              rule->getName().c_str());
                        }
                    }
                }

                if (!unit.hasSegments())
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
                ImGui::Text("Agent %s #%u", agent->getTypeName().c_str(), agent->getId());
                ImGui::TextDisabled("looking for %s",
                                    agent->getTarget().c_str());
            }
            break;
        }
        case game::Selection::Kind::Node:
            if (hover.node != nullptr)
            {
                ImGui::Text("Node #%u", hover.node->getId());
                if (hover.node->getSegments().empty())
                    ImGui::TextDisabled("orphan");
                for (Segment const* segment : hover.node->getSegments())
                {
                    ImGui::BulletText(
                        "%s, %.1f m", segment->getTypeName().c_str(), segment->getLength());
                }

                for (Unit const* unit : hover.node->getUnits())
                {
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                           theme::fromScript(unit->getColor())),
                                       "%s #%u stands here",
                                       unit->getTypeName().c_str(),
                                       unit->getId());
                }
            }
            break;
        case game::Selection::Kind::Segment:
            if (hover.segment != nullptr)
            {
                Segment const& segment = *hover.segment;
                ImGui::Text("Segment %s #%u", segment.getTypeName().c_str(), segment.getId());
                ImGui::Text("%.1f m", segment.getLength());
                ImGui::Text("free flow %.2f s, now %.2f s",
                            segment.getFreeFlowTime(),
                            segment.getTravelTime());
                float const saturation = segment.getSaturation();
                char overlay[64];
                std::snprintf(overlay,
                              sizeof(overlay),
                              "%.0f / %.0f agents",
                              segment.getFlow(),
                              segment.getCapacity());
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
        case game::Selection::Kind::Zone:
        {
            if (hover.zone == nullptr)
                break;
            CellRegion const& footprint = hover.zone->getRegion();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                   theme::fromScript(hover.zone->getColor())),
                               "Zone %s #%u",
                               hover.zone->getTypeName().c_str(),
                               hover.zone->getId());
            ImGui::TextDisabled(
                "%u x %u cells", footprint.sizeU, footprint.sizeV);
            ImGui::Text("%zu building(s) inside",
                        hover.zone->getUnitsInside().size());
            for (RuleZone const* rule : hover.zone->getRules())
            {
                if (rule != nullptr)
                    ImGui::BulletText("%s", rule->getName().c_str());
            }
            break;
        }
        case game::Selection::Kind::Cell:
        {
            auto const it = simulation.getCities().find(hover.city);
            if (it == simulation.getCities().end())
                break;
            City& city = *it->second;
            ImGui::TextUnformatted(city.getName().c_str());
            ImGui::Separator();
            ImGui::Text("cell (%d, %d)", hover.u, hover.v);
            if (city.getLayers().empty())
            {
                ImGui::TextDisabled("no layer");
            }
            else
            {
                for (auto const& layerIt : city.getLayers())
                {
                    Layer const& layer = *layerIt.second;
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(
                                           theme::fromScript(layer.getColor())),
                                       "%s",
                                       layer.getTypeName().c_str());
                    ImGui::SameLine();
                    ImGui::Text(": %u / %u",
                                layer.getResource({ hover.u, hover.v }),
                                layer.getCellCapacity());
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
    SimulationClock const& clock = simulation.getClock();
    float const hour =
        float(clock.getHourOfDay()) + float(clock.getMinuteOfHour()) / 60.0f;

    char time[16];
    std::snprintf(time,
                  sizeof(time),
                  "%02u:%02u",
                  clock.getHourOfDay(),
                  clock.getMinuteOfHour());
    char day[24];
    std::snprintf(day, sizeof(day), "Jour %u", clock.getDay());

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
