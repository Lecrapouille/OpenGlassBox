//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/GlassBoxApp.hpp"
#include "UI/Theme.hpp"

#include <implot.h>

#include <sys/stat.h>

#include <exception>
#include <iostream>

namespace ogb {

//! \brief Key of the file dialog opened by the File menu.
static constexpr char const* OPEN_SCRIPT_DIALOG = "OpenScriptDialog";
//! \brief Seconds between two checks of the script modification time. Often
//! enough to feel immediate, rare enough not to stat a file every frame.
static constexpr float WATCH_PERIOD = 0.5f;
//! \brief Seconds the reload notice stays in the status bar.
static constexpr float NOTICE_DURATION = 4.0f;

// ----------------------------------------------------------------------------
//! \brief Modification time of a file, or zero when it cannot be read.
// ----------------------------------------------------------------------------
static int64_t modificationTime(std::string const& path)
{
    struct stat info;
    if (path.empty() || (::stat(path.c_str(), &info) != 0))
        return 0;

    return int64_t(info.st_mtime);
}

// ----------------------------------------------------------------------------
GlassBoxApp::GlassBoxApp(Options options)
    : Application(options.width, options.height, "OpenGlassBox"),
      m_options(std::move(options)),
      m_data_path(DataPath::makeDefault(m_options.dataPath))
{}

// ----------------------------------------------------------------------------
GlassBoxApp::~GlassBoxApp()
{
    // The trace holds a pointer registered globally in the engine; drop it
    // before the panels that read it are destroyed.
    m_trace.setRecording(false);
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::onSetup()
{
    theme::loadFonts(m_data_path);
    ImPlot::CreateContext();
    imgui().setClearColor(0.05f, 0.06f, 0.08f);
    imgui().setDefaultLayoutCallback(
        [this](ImGuiID dockspace) { this->buildDefaultLayout(dockspace); });

    if (!loadScript(m_options.script))
    {
        // Not fatal: the window opens on an empty world and the File menu lets
        // another script be picked, which beats exiting with a message.
        std::cerr << "Warning: " << m_script_error << std::endl;
    }

    return true;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onTeardown()
{
    ImPlot::DestroyContext();
}

// ----------------------------------------------------------------------------
void GlassBoxApp::buildDefaultLayout(ImGuiID dockspace)
{
    // The map keeps the center and everything else is arranged around it. Each
    // split narrows the remaining center, so the fractions are relative to what
    // is left at that point. Both children have to be collected: docking into
    // the node that was split would target the parent, which is not a leaf and
    // silently swallows the window.
    ImGuiID center = dockspace;
    ImGuiID left = 0u;
    ImGuiID right = 0u;
    ImGuiID bottom = 0u;
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20f, &left, &center);
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, &right, &center);
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.32f, &bottom, &center);

    ImGuiID leftTop = 0u;
    ImGuiID leftBottom = 0u;
    ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.62f, &leftBottom, &leftTop);

    ImGuiID rightTop = 0u;
    ImGuiID rightBottom = 0u;
    ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.50f, &rightBottom, &rightTop);

    ImGui::DockBuilderDockWindow("Map", center);
    ImGui::DockBuilderDockWindow("Time", leftTop);
    ImGui::DockBuilderDockWindow("Layers", leftBottom);
    ImGui::DockBuilderDockWindow("Inspector", rightTop);
    ImGui::DockBuilderDockWindow("Traffic", rightBottom);
    ImGui::DockBuilderDockWindow("History", rightBottom);
    // Tabbed together: both answer "what is the simulation doing over time".
    ImGui::DockBuilderDockWindow("Rule Log", bottom);
    ImGui::DockBuilderDockWindow("Charts", bottom);
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::loadScript(std::string const& filename)
{
    auto const found = m_data_path.find(filename);
    if (!found.second)
    {
        m_script_error = "Cannot find '" + filename + "' in the data path " +
                         m_data_path.toString();
        return false;
    }

    auto simulation = std::make_unique<Simulation>(12u, 12u);

    try
    {
        if (!simulation->script().parse(found.first))
        {
            m_script_error = "Failed parsing '" + found.first + "'";
            return false;
        }
    }
    catch (std::exception const& e)
    {
        m_script_error = "Failed parsing '" + found.first + "': " + e.what();
        return false;
    }

    m_simulation = std::move(simulation);
    m_script_path = found.first;
    m_script_error.clear();

    m_state.selection.clear();
    m_charts.clear();
    m_trace.clear();
    // The recorded edits describe a world that no longer exists.
    m_editor.reset();

    try
    {
        buildDemoWorld();
    }
    catch (std::exception const& e)
    {
        m_script_error = "The script parsed but does not define what the demo "
                         "world needs: " + std::string(e.what());
        return false;
    }

    // Show the first map as the main heatmap, so that the view is not empty
    // and the user immediately sees what a layer looks like.
    if (m_state.primaryLayer.empty() && !m_simulation->cities().empty())
    {
        auto const& maps = m_simulation->cities().begin()->second->maps();
        if (!maps.empty())
        {
            m_state.primaryLayer = maps.begin()->second->type();
        }
    }

    m_viewer.requestFrameAll();
    m_script_mtime = modificationTime(m_script_path);
    setTitle("OpenGlassBox - " + m_script_path);

    return true;
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::reloadScript()
{
    if (m_script_path.empty())
        return false;

    // Take the history away from the editor before it is cleared by the load,
    // then apply it to the world that comes out of the reparse.
    std::deque<CommandPtr> edits;
    m_editor.stack().takeHistory(edits);

    bool const loaded = loadScript(m_script_path);
    if (!loaded)
    {
        return false;
    }

    size_t replayed = 0u;
    for (auto& command: edits)
    {
        command->onWorldRebuilt();

        // A command can legitimately fail to apply again, for instance when the
        // new script dropped the type of road it was laying. Skipping it is
        // better than refusing the whole reload.
        if (m_editor.stack().push(*m_simulation, std::move(command)))
        {
            ++replayed;
        }
    }

    m_reload_notice = "script reloaded";
    if (!edits.empty())
    {
        m_reload_notice += ", " + std::to_string(replayed) + "/" +
                           std::to_string(edits.size()) + " edits replayed";
    }
    m_reload_notice_timer = NOTICE_DURATION;

    return true;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::watchScriptFile(float dt)
{
    if (!m_auto_reload || m_script_path.empty())
        return;

    m_watch_timer -= dt;
    if (m_watch_timer > 0.0f)
        return;

    m_watch_timer = WATCH_PERIOD;

    int64_t const mtime = modificationTime(m_script_path);
    if ((mtime == 0) || (mtime == m_script_mtime))
        return;

    reloadScript();
}

// ----------------------------------------------------------------------------
void GlassBoxApp::openScriptDialog()
{
    FileDialogRequest request;
    request.key = OPEN_SCRIPT_DIALOG;
    request.title = "Open a simulation script";
    request.filters = ".txt,.*";
    request.startPath = ".";
    request.onAccepted = [this](std::string const& path) {
        loadScript(path);
    };

    imgui().requestFileDialog(std::move(request));
}

// ----------------------------------------------------------------------------
void GlassBoxApp::buildDemoWorld()
{
    Simulation& simulation = *m_simulation;
    Script& script = simulation.script();

    auto addAllMaps = [&](City& city) {
        for (auto const& it: script.mapTypes())
            city.addMap(*it.second);
    };

    bool const braess = (script.definitions().unitTypes().find("Start") !=
                         script.definitions().unitTypes().end()) &&
                        (script.definitions().unitTypes().find("End") !=
                         script.definitions().unitTypes().end());

    if (braess)
    {
        City& city = simulation.addCity("Braess", Vector3f(0.0f, 0.0f, 0.0f),
                                        16u, 12u);
        addAllMaps(city);

        Path& roads = city.addPath(script.getPathType("Road"));
        WayType const& fast = script.getWayType("Fast");
        WayType const& slow = script.getWayType("Slow");
        WayType const& shortcut = script.getWayType("Shortcut");

        Node& start = roads.addNode(Vector3f(40.0f, 180.0f, 0.0f));
        Node& a = roads.addNode(Vector3f(200.0f, 60.0f, 0.0f));
        Node& b = roads.addNode(Vector3f(200.0f, 300.0f, 0.0f));
        Node& end = roads.addNode(Vector3f(360.0f, 180.0f, 0.0f));

        roads.addWay(fast, start, a);
        roads.addWay(slow, start, b);
        roads.addWay(slow, a, end);
        roads.addWay(fast, b, end);
        roads.addWay(shortcut, a, b);

        city.addUnit(script.getUnitType("Start"), start);
        city.addUnit(script.getUnitType("End"), end);
        return;
    }

    City& paris = simulation.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    addAllMaps(paris);

    Path& parisRoads = paris.addPath(script.getPathType("Road"));
    WayType const& dirt = script.getWayType("Dirt");

    Node& p1 = parisRoads.addNode(Vector3f(40.0f, 40.0f, 0.0f));
    Node& p2 = parisRoads.addNode(Vector3f(320.0f, 40.0f, 0.0f));
    Node& p3 = parisRoads.addNode(Vector3f(320.0f, 320.0f, 0.0f));
    Node& p4 = parisRoads.addNode(Vector3f(40.0f, 320.0f, 0.0f));
    Node& p5 = parisRoads.addNode(Vector3f(180.0f, 180.0f, 0.0f));

    Way& pw1 = parisRoads.addWay(dirt, p1, p2);
    Way& pw2 = parisRoads.addWay(dirt, p2, p3);
    Way& pw3 = parisRoads.addWay(dirt, p3, p4);
    Way& pw4 = parisRoads.addWay(dirt, p4, p1);
    parisRoads.addWay(dirt, p1, p5);
    parisRoads.addWay(dirt, p5, p3);

    UnitType const& home = script.getUnitType("Home");
    UnitType const& work = script.getUnitType("Work");

    paris.addUnit(home, parisRoads, pw1, 0.25f);
    paris.addUnit(home, parisRoads, pw1, 0.60f);
    paris.addUnit(home, parisRoads, pw4, 0.50f);
    paris.addUnit(work, parisRoads, pw2, 0.50f);
    paris.addUnit(work, parisRoads, pw3, 0.40f);

    try
    {
        UnitType const& shop = script.getUnitType("Shop");
        paris.addUnit(shop, parisRoads, pw3, 0.75f);
    }
    catch (...)
    {}

    try
    {
        paris.addArea(script.getAreaType("Residential"), paris.region());
    }
    catch (...)
    {}

    float const parisWidth = float(paris.gridSizeU()) * paris.gridCellSize();
    Vector3f const versaillesOrigin(parisWidth + 80.0f, 0.0f, 0.0f);

    City& versailles = simulation.addCity("Versailles", versaillesOrigin);
    addAllMaps(versailles);

    Path& versaillesRoads = versailles.addPath(script.getPathType("Road"));
    Node& v1 = versaillesRoads.addNode(versaillesOrigin + Vector3f(60.0f, 60.0f, 0.0f));
    Node& v2 = versaillesRoads.addNode(versaillesOrigin + Vector3f(280.0f, 90.0f, 0.0f));
    Node& v3 = versaillesRoads.addNode(versaillesOrigin + Vector3f(160.0f, 300.0f, 0.0f));

    Way& vw1 = versaillesRoads.addWay(dirt, v1, v2);
    Way& vw2 = versaillesRoads.addWay(dirt, v2, v3);
    versaillesRoads.addWay(dirt, v3, v1);

    versailles.addUnit(home, versaillesRoads, vw1, 0.35f);
    versailles.addUnit(work, versaillesRoads, vw2, 0.55f);

    try
    {
        versailles.addArea(script.getAreaType("Commercial"), versailles.region());
    }
    catch (...)
    {}
}

// ----------------------------------------------------------------------------
void GlassBoxApp::advanceSimulation(float dt)
{
    if (!m_simulation)
        return;

    uint64_t const before = m_simulation->totalTicks();

    uint32_t steps = m_time.takePendingSteps();
    while (steps-- > 0u)
    {
        m_trace.setTick(m_simulation->totalTicks());
        m_simulation->stepOneTick();
        m_charts.sample(*m_simulation);
    }

    // The engine consumes the elapsed time in whole ticks, so tell the trace
    // which tick it is stamping before handing over.
    m_trace.setTick(m_simulation->totalTicks());
    m_simulation->update(dt);

    m_ticks_last_frame = m_simulation->totalTicks() - before;
    if (m_ticks_last_frame != 0u)
    {
        m_charts.sample(*m_simulation);
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onUpdate(float dt)
{
    watchScriptFile(dt);
    advanceSimulation(dt);

    if (m_reload_notice_timer > 0.0f)
    {
        m_reload_notice_timer -= dt;
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onDrawMenuBar()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open simulation script...", "Ctrl+O"))
        {
            openScriptDialog();
        }

        if (ImGui::MenuItem("Reload", "F5", false, !m_script_path.empty()))
        {
            reloadScript();
        }

        ImGui::MenuItem("Reload when the file changes", nullptr, &m_auto_reload);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Watch the script on disk and reload it when it is saved.\n"
                "The world is rebuilt, and the roads and buildings placed by\n"
                "hand are replayed on top of it.");
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Alt+F4"))
        {
            halt();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        CommandStack& stack = m_editor.stack();

        if (ImGui::MenuItem(stack.canUndo()
                                ? ("Undo " + stack.undoLabel()).c_str()
                                : "Undo",
                            "Ctrl+Z", false, stack.canUndo()) &&
            m_simulation)
        {
            m_editor.undo(*m_simulation);
        }
        if (ImGui::MenuItem(stack.canRedo()
                                ? ("Redo " + stack.redoLabel()).c_str()
                                : "Redo",
                            "Ctrl+Y", false, stack.canRedo()) &&
            m_simulation)
        {
            m_editor.redo(*m_simulation);
        }

        ImGui::Separator();

        struct Entry { EditTool tool; char const* label; char const* shortcut; };
        static Entry const TOOLS[] = {
            { EditTool::Select, "Select", "1" },
            { EditTool::Road, "Lay a road", "2" },
            { EditTool::Building, "Place a building", "3" },
            { EditTool::Zone, "Paint a zone", "4" },
            { EditTool::Paint, "Paint a resource", "5" },
            { EditTool::Bulldozer, "Bulldoze", "6" },
        };
        for (auto const& entry: TOOLS)
        {
            if (ImGui::MenuItem(entry.label, entry.shortcut,
                                m_editor.tool() == entry.tool))
            {
                m_editor.setTool(entry.tool);
            }
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Layers", nullptr, &m_show_layers);
        ImGui::MenuItem("Inspector", nullptr, &m_show_inspector);
        ImGui::MenuItem("Rule Log", nullptr, &m_show_rule_log);
        ImGui::MenuItem("Charts", nullptr, &m_show_charts);
        ImGui::MenuItem("Time", nullptr, &m_show_time);
        ImGui::MenuItem("Traffic", nullptr, &m_show_traffic);
        ImGui::MenuItem("History", nullptr, &m_show_history);
        ImGui::Separator();
        if (ImGui::MenuItem("Recenter the map", "Home"))
        {
            m_viewer.requestFrameAll();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About"))
        {
            m_show_about = true;
        }
        ImGui::EndMenu();
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onDrawPanels()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
    {
        openScriptDialog();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F5, false) && !m_script_path.empty())
    {
        reloadScript();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false))
    {
        m_viewer.requestFrameAll();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !io.WantTextInput &&
        m_simulation)
    {
        m_simulation->setPaused(!m_simulation->paused());
    }

    if (!io.WantTextInput && !io.KeyCtrl)
    {
        static EditTool const SHORTCUTS[] = {
            EditTool::Select, EditTool::Road, EditTool::Building,
            EditTool::Zone, EditTool::Paint, EditTool::Bulldozer,
        };
        for (int i = 0; i < IM_ARRAYSIZE(SHORTCUTS); ++i)
        {
            if (ImGui::IsKeyPressed(ImGuiKey(int(ImGuiKey_1) + i), false))
            {
                m_editor.setTool(SHORTCUTS[i]);
            }
        }
    }

    if (m_simulation)
    {
        m_viewer.draw(*m_simulation, m_state, m_editor);

        if (m_show_history)
        {
            m_editor.drawHistoryPanel(*m_simulation);
        }
        if (m_show_time)
        {
            m_time.draw(*m_simulation, m_state, m_trace);
        }
        if (m_show_layers)
        {
            m_layers.draw(*m_simulation, m_state);
        }
        if (m_show_inspector)
        {
            m_inspector.draw(*m_simulation, m_state, m_trace);
        }
        if (m_show_rule_log)
        {
            m_rule_log.draw(*m_simulation, m_state, m_trace);
        }
        if (m_show_charts)
        {
            m_charts.draw(*m_simulation, m_state);
        }
        if (m_show_traffic)
        {
            m_traffic.draw(*m_simulation, m_state);
        }
    }

    drawScriptError();
    drawAboutPopup();
}

// ----------------------------------------------------------------------------
void GlassBoxApp::drawScriptError()
{
    if (m_script_error.empty())
        return;

    ImGui::OpenPopup("Script error");
    if (ImGui::BeginPopupModal("Script error", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "The simulation script could not be loaded.");
        ImGui::Separator();
        ImGui::PushTextWrapPos(520.0f);
        ImGui::TextUnformatted(m_script_error.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            m_script_error.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::drawAboutPopup()
{
    if (!m_show_about)
        return;

    ImGui::OpenPopup("About OpenGlassBox");
    if (ImGui::BeginPopupModal("About OpenGlassBox", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("OpenGlassBox");
        ImGui::TextDisabled("An open implementation of the GlassBox engine\n"
                            "of SimCity 2013.");
        ImGui::Separator();
        ImGui::TextUnformatted("Mouse");
        ImGui::BulletText("left: select a unit, an agent, a node or a cell");
        ImGui::BulletText("middle or right drag: pan");
        ImGui::BulletText("wheel: zoom on the cursor");
        ImGui::TextUnformatted("Keyboard");
        ImGui::BulletText("space: pause and resume");
        ImGui::BulletText("Home: fit the whole world in the view");
        ImGui::BulletText("F5: reload the simulation script");
        ImGui::BulletText("Ctrl+O: open another script");
        ImGui::BulletText("1 to 5: select, road, build, paint, bulldoze");
        ImGui::BulletText("Ctrl+Z, Ctrl+Y: undo and redo an edit");
        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            m_show_about = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onDrawStatusBar()
{
    ImGuiIO const& io = ImGui::GetIO();

    ImGui::Text("%.0f fps", io.Framerate);
    ImGui::SameLine(0.0f, 20.0f);

    if (!m_simulation)
    {
        ImGui::TextDisabled("no simulation loaded");
        return;
    }

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(
            m_simulation->paused() ? theme::FAILURE : theme::SUCCESS),
        "%s", m_simulation->paused() ? "paused" : "running");

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::Text("tick %llu (%llu this frame, x%.2f)",
                (unsigned long long)m_simulation->totalTicks(),
                (unsigned long long)m_ticks_last_frame,
                m_simulation->timeScale());

    size_t units = 0u;
    size_t agents = 0u;
    for (auto const& it: m_simulation->cities())
    {
        units += it.second->units().size();
        agents += it.second->agents().size();
    }

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::Text("%zu cities, %zu units, %zu agents",
                m_simulation->cities().size(), units, agents);

    if (m_state.hasHoveredCell)
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextDisabled("%s (%d, %d)", m_state.hoveredCity.c_str(),
                            m_state.hoveredU, m_state.hoveredV);
    }

    std::string const hint = m_editor.hint();
    if (!hint.empty())
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::ACCENT), "%s",
                           hint.c_str());
    }

    if (m_reload_notice_timer > 0.0f)
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::SUCCESS), "%s",
                           m_reload_notice.c_str());
    }

    if (!m_script_path.empty())
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextDisabled("%s", m_script_path.c_str());
    }
}

} // namespace ogb
