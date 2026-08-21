//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/CityViewer.hpp"
#include "UI/Theme.hpp"
#include "Core/Editor.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace ogb {

//! \brief Bounds of the zoom, in pixels per world unit.
static constexpr float MIN_ZOOM = 0.05f;
static constexpr float MAX_ZOOM = 20.0f;
//! \brief How far, in pixels, a click may land from an entity to select it.
static constexpr float PICK_RADIUS = 12.0f;
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

    for (auto const& it: simulation.cities())
    {
        City const& city = *it.second;

        // The grid of the city always bounds its maps, and the nodes may stick
        // out of it, so take the union of both.
        float const side = city.gridCellSize();
        minX = std::min(minX, city.position().x);
        minY = std::min(minY, city.position().y);
        maxX = std::max(maxX, city.position().x + float(city.gridSizeU()) * side);
        maxY = std::max(maxY, city.position().y + float(city.gridSizeV()) * side);
        found = true;

        for (auto const& path: city.paths())
        {
            for (auto const& node: path.second->nodes())
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
void CityViewer::draw(Simulation& simulation, DebugState& state, Editor& editor)
{
    if (!ImGui::Begin("Map"))
    {
        ImGui::End();
        return;
    }

    drawToolbar(simulation, state, editor);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::CANVAS_BACKGROUND);
    ImGui::BeginChild("Canvas", ImVec2(0.0f, 0.0f), true,
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
    m_draw_list->PushClipRect(m_canvas_origin,
                              m_canvas_origin + m_canvas_size, true);

    handleInputs(simulation, state, editor);

    m_splitter.Split(m_draw_list, CHANNEL_COUNT);

    // The maps belong to the world and span every city, so they are drawn once
    // rather than once per city.
    drawMaps(simulation.world(), state);

    for (auto& it: simulation.cities())
    {
        City& city = *it.second;
        drawCityFrame(city, state);
        if (state.showAreas)
            drawAreas(city);
        drawPaths(city, state);
        drawUnits(city, state);
        drawAgents(city, state);
    }

    drawSelectionOverlay(simulation, state);

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_OVERLAY);
    editor.drawPreview(simulation, *this, m_draw_list);

    m_splitter.Merge(m_draw_list);

    m_draw_list->PopClipRect();

    drawLegend(state);
    drawHoverTooltip(simulation, state);

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();
}

// ----------------------------------------------------------------------------
void CityViewer::drawToolbar(Simulation& simulation, DebugState& state,
                             Editor& editor)
{
    editor.drawToolbar(simulation, state);

    if (ImGui::Button("Recenter"))
    {
        frameAll(simulation);
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(140.0f);
    ImGui::SliderFloat("Zoom", &m_zoom, MIN_ZOOM, MAX_ZOOM, "%.2f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine();

    ImGui::Checkbox("Grid", &state.showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Paths", &state.showPaths);
    ImGui::SameLine();
    ImGui::Checkbox("Units", &state.showUnits);
    ImGui::SameLine();
    ImGui::Checkbox("Areas", &state.showAreas);
    ImGui::SameLine();
    ImGui::Checkbox("Agents", &state.showAgents);
    ImGui::SameLine();
    ImGui::Checkbox("Traffic", &state.showTraffic);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Color and thicken the roads by their flow over\n"
                          "capacity ratio, from green to red.");
    }
    ImGui::SameLine();
    ImGui::Checkbox("Labels", &state.showLabels);

    ImGui::Separator();
}

// ----------------------------------------------------------------------------
void CityViewer::handleInputs(Simulation& simulation, DebugState& state,
                              Editor& editor)
{
    // The child window is not a widget, so claim the area explicitly to know
    // whether the mouse belongs to us.
    ImGui::InvisibleButton("CanvasSurface", m_canvas_size,
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight |
                           ImGuiButtonFlags_MouseButtonMiddle);

    bool const hovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();

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
    updateHover(simulation, state);

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
static float distanceToSegment(ImVec2 const& point, Vector3f const& a,
                               Vector3f const& b, float& offset)
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
Way* CityViewer::pickWay(City& city, ImVec2 const& world, float pixels,
                         float& offset) const
{
    // Work in world units so that the tolerance stays the same on screen
    // whatever the zoom.
    float const tolerance = pixels / std::max(1e-3f, m_zoom);
    float best = tolerance * tolerance;
    Way* found = nullptr;

    for (auto& it: city.paths())
    {
        for (auto& way: it.second->ways())
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

    for (auto& it: city.paths())
    {
        for (auto& node: it.second->nodes())
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

    for (auto& unit: city.units())
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
void CityViewer::pick(Simulation& simulation, DebugState& state,
                      ImVec2 const& screen)
{
    float bestDistance = PICK_RADIUS * PICK_RADIUS;
    Selection best;

    auto const consider = [&](ImVec2 const& position, Selection candidate) {
        float const dx = position.x - screen.x;
        float const dy = position.y - screen.y;
        float const distance = dx * dx + dy * dy;
        if (distance <= bestDistance)
        {
            bestDistance = distance;
            best = std::move(candidate);
        }
    };

    for (auto& it: simulation.cities())
    {
        City& city = *it.second;

        if (state.showUnits)
        {
            for (auto& unit: city.units())
            {
                Selection candidate;
                candidate.kind = Selection::Kind::Unit;
                candidate.city = city.name();
                candidate.unit = unit.get();
                consider(worldToScreen(unit->position()), candidate);
            }
        }

        if (state.showAgents)
        {
            for (auto& agent: city.agents())
            {
                Selection candidate;
                candidate.kind = Selection::Kind::Agent;
                candidate.city = city.name();
                candidate.agentId = agent->id();
                consider(worldToScreen(agent->position()), candidate);
            }
        }

        if (state.showNodes && state.showPaths)
        {
            for (auto& path: city.paths())
            {
                for (auto& node: path.second->nodes())
                {
                    Selection candidate;
                    candidate.kind = Selection::Kind::Node;
                    candidate.city = city.name();
                    candidate.node = node.get();
                    consider(worldToScreen(node->position()), candidate);
                }
            }
        }
    }

    if (best.kind != Selection::Kind::None)
    {
        state.selection = std::move(best);
        return;
    }

    // Nothing close enough: fall back on the grid cell, which is still useful
    // to read the maps.
    if (state.hasHoveredCell)
    {
        Selection cell;
        cell.kind = Selection::Kind::Cell;
        cell.city = state.hoveredCity;
        cell.u = state.hoveredU;
        cell.v = state.hoveredV;
        state.selection = cell;
    }
    else
    {
        state.selection.clear();
    }
}

// ----------------------------------------------------------------------------
void CityViewer::updateHover(Simulation& simulation, DebugState& state)
{
    state.hasHoveredCell = false;

    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        return;

    ImVec2 const world = screenToWorld(ImGui::GetIO().MousePos);

    int32_t u, v;
    simulation.world().world2mapPosition(Vector3f(world.x, world.y, 0.0f), u, v);

    for (auto& it: simulation.cities())
    {
        City& city = *it.second;
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
void CityViewer::drawMaps(World& world, DebugState const& state)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_MAPS);

    float const side = world.cellSize();
    float const pixels = side * m_zoom;

    for (auto& it: world.maps())
    {
        Map& map = *it.second;
        if (!state.isLayerVisible(map.type()))
            continue;

        auto const settings = state.layers.find(map.type());
        LayerSettings const options =
            (settings != state.layers.end()) ? settings->second : LayerSettings();

        bool const primary = (state.primaryLayer == map.type()) ||
                             (state.soloLayer == map.type());
        uint32_t const capacity = std::max(1u, map.getCapacity());

        // Only the cells that hold something are stored, and only those are
        // worth drawing.
        map.forEachCell([&](int32_t u, int32_t v, uint32_t amount) {
                float const ratio =
                    std::min(1.0f, float(amount) / float(capacity));

                ImVec2 const p0 =
                    worldToScreen(float(u) * side, float(v) * side);
                ImVec2 const p1(p0.x + pixels, p0.y + pixels);

                switch (options.mode)
                {
                case LayerMode::Heatmap:
                {
                    // A non primary map is drawn thinner so that several of
                    // them stay readable when superimposed.
                    float const inset = primary ? 0.0f : pixels * 0.18f;
                    m_draw_list->AddRectFilled(
                        ImVec2(p0.x + inset, p0.y + inset),
                        ImVec2(p1.x - inset, p1.y - inset),
                        theme::fromScript(map.color(), ratio * options.opacity));
                    break;
                }
                case LayerMode::Contour:
                    m_draw_list->AddRect(
                        p0, p1,
                        theme::fromScript(map.color(), options.opacity),
                        0.0f, 0, 1.0f + 2.0f * ratio);
                    break;

                case LayerMode::Value:
                {
                    if (pixels < 22.0f)
                        break;
                    char label[16];
                    std::snprintf(label, sizeof(label), "%u", amount);
                    ImVec2 const size = ImGui::CalcTextSize(label);
                    m_draw_list->AddText(
                        ImVec2(0.5f * (p0.x + p1.x) - 0.5f * size.x,
                               0.5f * (p0.y + p1.y) - 0.5f * size.y),
                        theme::fromScript(map.color(), options.opacity), label);
                    break;
                }
                }
        });
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawCityFrame(City& city, DebugState const& state)
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
                                     ImVec2(x, bottomRight.y), theme::GRID_LINE);
            }
            for (uint32_t v = 1u; v < city.gridSizeV(); ++v)
            {
                float const y = topLeft.y + float(v) * pixels;
                m_draw_list->AddLine(ImVec2(topLeft.x, y),
                                     ImVec2(bottomRight.x, y), theme::GRID_LINE);
            }
        }
    }

    m_draw_list->AddRect(topLeft, bottomRight, theme::GRID_LINE_STRONG, 0.0f, 0, 2.0f);

    if (state.showLabels)
    {
        m_draw_list->AddText(ImVec2(topLeft.x + 4.0f, topLeft.y - 18.0f),
                             IM_COL32(200, 205, 215, 220), city.name().c_str());
    }

    if (state.hasHoveredCell && (state.hoveredCity == city.name()))
    {
        ImVec2 const p0 = worldToScreen(
            city.world().mapPosition2world(state.hoveredU, state.hoveredV));
        ImVec2 const p1(p0.x + side * m_zoom, p0.y + side * m_zoom);
        m_draw_list->AddRect(p0, p1, IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.5f);
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawAreas(City& city)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_GRID);

    float const side = city.gridCellSize();
    for (auto& area: city.areas())
    {
        MapRegion const& region = area->footprint();
        Vector3f const topLeftWorld = city.world().mapPosition2world(region.u0, region.v0);
        ImVec2 const p0 = worldToScreen(topLeftWorld);
        ImVec2 const p1 = worldToScreen(
            topLeftWorld.x + float(region.sizeU) * side,
            topLeftWorld.y + float(region.sizeV) * side);
        ImU32 const fill = theme::fromScript(area->color(), 0.18f);
        ImU32 const line = theme::fromScript(area->color(), 0.70f);
        m_draw_list->AddRectFilled(p0, p1, fill);
        m_draw_list->AddRect(p0, p1, line, 0.0f, 0, 2.0f);
        if (m_zoom > 0.4f)
        {
            m_draw_list->AddText(ImVec2(p0.x + 4.0f, p0.y + 2.0f), line,
                                 area->type().c_str());
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawPaths(City& city, DebugState const& state)
{
    if (!state.showPaths)
        return;

    bool const details = m_zoom > DETAIL_ZOOM_THRESHOLD;

    for (auto& it: city.paths())
    {
        Path& path = *it.second;

        m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_WAYS);
        for (auto& way: path.ways())
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
        for (auto& node: path.nodes())
        {
            ImVec2 const position = worldToScreen(node->position());
            m_draw_list->AddCircleFilled(position, NODE_RADIUS,
                                         IM_COL32(200, 205, 215, 220));
            m_draw_list->AddCircle(position, NODE_RADIUS,
                                   IM_COL32(20, 22, 26, 255), 0, 1.5f);
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawUnits(City& city, DebugState const& state)
{
    if (!state.showUnits)
        return;

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_UNITS);

    bool const labels = state.showLabels && (m_zoom > LABEL_ZOOM_THRESHOLD);

    for (auto& unit: city.units())
    {
        ImVec2 const position = worldToScreen(unit->position());
        ImU32 const color = theme::fromScript(unit->color());

        // A square, so that a Unit is never confused with a Node or an Agent.
        m_draw_list->AddRectFilled(
            ImVec2(position.x - UNIT_RADIUS, position.y - UNIT_RADIUS),
            ImVec2(position.x + UNIT_RADIUS, position.y + UNIT_RADIUS),
            color, 2.0f);
        m_draw_list->AddRect(
            ImVec2(position.x - UNIT_RADIUS, position.y - UNIT_RADIUS),
            ImVec2(position.x + UNIT_RADIUS, position.y + UNIT_RADIUS),
            IM_COL32(15, 16, 20, 255), 2.0f, 0, 1.5f);

        if (labels)
        {
            m_draw_list->AddText(
                ImVec2(position.x + UNIT_RADIUS + 3.0f, position.y - 7.0f),
                IM_COL32(220, 225, 235, 220), unit->type().c_str());
        }
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawAgents(City& city, DebugState const& state)
{
    if (!state.showAgents)
        return;

    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_AGENTS);

    for (auto& agent: city.agents())
    {
        ImVec2 const position = worldToScreen(agent->position());
        m_draw_list->AddCircleFilled(position, AGENT_RADIUS,
                                     theme::fromScript(agent->color()));
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawSelectionOverlay(Simulation& simulation,
                                      DebugState const& state)
{
    m_splitter.SetCurrentChannel(m_draw_list, CHANNEL_OVERLAY);

    Selection const& selection = state.selection;
    if (selection.kind == Selection::Kind::None)
        return;

    auto const cityIt = simulation.cities().find(selection.city);
    if (cityIt == simulation.cities().end())
        return;
    City& city = *cityIt->second;

    ImU32 const highlight = theme::ACCENT;

    switch (selection.kind)
    {
    case Selection::Kind::Unit:
    {
        if (selection.unit == nullptr)
            break;

        ImVec2 const position = worldToScreen(selection.unit->position());
        m_draw_list->AddCircle(position, UNIT_RADIUS + 6.0f, highlight, 0, 2.0f);

        if (state.showSelectionRadius)
        {
            // Materialize the disc the rules of this Unit read and write on the
            // maps. Seeing it is the quickest way to understand why a Unit does
            // not reach the resource next door.
            float const radius = float(selection.unit->mapRadius()) *
                                 city.gridCellSize() * m_zoom;
            if (radius > 1.0f)
            {
                m_draw_list->AddCircle(position, radius,
                                       theme::fromScript(0x56AADE, 0.7f), 0, 1.5f);
            }
        }
        break;
    }
    case Selection::Kind::Node:
    {
        if (selection.node == nullptr)
            break;
        m_draw_list->AddCircle(worldToScreen(selection.node->position()),
                               NODE_RADIUS + 6.0f, highlight, 0, 2.0f);
        break;
    }
    case Selection::Kind::Agent:
    {
        Agent const* const agent = selection.resolveAgent(simulation);
        if (agent == nullptr)
            break;

        ImVec2 const position = worldToScreen(agent->position());
        m_draw_list->AddCircle(position, AGENT_RADIUS + 6.0f, highlight, 0, 2.0f);

        Route const& route = agent->route();
        if (route.found)
        {
            ImVec2 previous = position;
            for (Node* node: route.nodes)
            {
                ImVec2 const next = worldToScreen(node->position());
                m_draw_list->AddLine(previous, next, highlight, 2.0f);
                previous = next;
            }
            if (route.destination != nullptr)
            {
                m_draw_list->AddLine(previous,
                                     worldToScreen(route.destination->position()),
                                     highlight, 2.0f);
            }
        }
        else
        {
            for (auto& unit: city.units())
            {
                if (unit->type() != agent->searchTarget())
                    continue;

                m_draw_list->AddLine(position, worldToScreen(unit->position()),
                                     theme::fromScript(0x56AADE, 0.45f), 1.5f);
            }
        }
        break;
    }
    case Selection::Kind::Cell:
    {
        float const side = city.gridCellSize();
        ImVec2 const p0 = worldToScreen(
            city.world().mapPosition2world(selection.u, selection.v));
        ImVec2 const p1(p0.x + side * m_zoom, p0.y + side * m_zoom);
        m_draw_list->AddRect(p0, p1, highlight, 0.0f, 0, 2.5f);
        break;
    }
    case Selection::Kind::None:
        break;
    }
}

// ----------------------------------------------------------------------------
void CityViewer::drawLegend(DebugState const& state)
{
    if (!state.showTraffic)
        return;

    // Anchored to the bottom left corner of the canvas.
    float const width = 140.0f;
    float const height = 10.0f;
    ImVec2 const origin(m_canvas_origin.x + 12.0f,
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

    m_draw_list->AddText(ImVec2(origin.x, origin.y - 16.0f), theme::MUTED,
                         "free");
    m_draw_list->AddText(ImVec2(origin.x + width - 44.0f, origin.y - 16.0f),
                         theme::MUTED, "jammed");
}

// ----------------------------------------------------------------------------
void CityViewer::drawHoverTooltip(Simulation& simulation, DebugState const& state)
{
    if (!state.hasHoveredCell)
        return;

    auto const it = simulation.cities().find(state.hoveredCity);
    if (it == simulation.cities().end())
        return;

    City& city = *it->second;

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(city.name().c_str());
    ImGui::Separator();
    ImGui::Text("cell (%d, %d)", state.hoveredU, state.hoveredV);

    if (city.maps().empty())
    {
        ImGui::TextDisabled("no map");
    }
    else
    {
        for (auto& mapIt: city.maps())
        {
            Map& map = *mapIt.second;
            ImGui::TextColored(
                ImGui::ColorConvertU32ToFloat4(theme::fromScript(map.color())),
                "%s", map.type().c_str());
            ImGui::SameLine();
            ImGui::Text(": %u / %u", map.getResource(state.hoveredU, state.hoveredV),
                        map.getCapacity());
        }
    }

    ImGui::EndTooltip();
}

} // namespace ogb
