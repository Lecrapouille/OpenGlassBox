//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Game/GlassBoxApp.hpp"
#include "UI/Theme.hpp"
#include "OpenGlassBox/CitySave.hpp"

#include <implot.h>

#include <sys/stat.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "project_info.hpp"

namespace ogb {
namespace game {
using namespace ogb::theme;


namespace {

static constexpr char const* OPEN_RULESET_DIALOG = "OpenRulesetDialog";
static constexpr char const* OPEN_CITY_DIALOG = "OpenCityDialog";
static constexpr char const* SAVE_CITY_DIALOG = "SaveCityDialog";
static constexpr float WATCH_PERIOD = 0.5f;
static constexpr float NOTICE_DURATION = 4.0f;
static constexpr uint32_t DEFAULT_CITY_SIZE = 16u;

bool fileExists(std::string const& path)
{
    struct stat info;
    return !path.empty() && (::stat(path.c_str(), &info) == 0);
}

std::string fileBasename(std::string const& path)
{
    size_t const slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1u);
}

std::string fileDirectory(std::string const& path)
{
    size_t const slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? std::string() : path.substr(0u, slash + 1u);
}

std::string fileExtension(std::string const& path)
{
    std::string const name = fileBasename(path);
    size_t const dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return {};
    return name.substr(dot);
}

std::string fileStem(std::string const& path)
{
    std::string const name = fileBasename(path);
    size_t const dot = name.find_last_of('.');
    return (dot == std::string::npos) ? name : name.substr(0u, dot);
}

std::string joinPath(std::string dir, std::string const& name)
{
    if (!dir.empty() && (dir.back() != '/') && (dir.back() != '\\'))
        dir += '/';
    return dir + name;
}

std::string dataDirectory()
{
    std::string dir = project::info::paths::data;
    if (!dir.empty() && (dir.back() != '/') && (dir.back() != '\\'))
        dir += '/';
    return dir;
}

//! \brief As given if the path exists, otherwise Simulations/ under the
//! build-time data path.
std::string resolveDataFile(std::string const& name)
{
    if (fileExists(name))
        return name;

    std::string const data = dataDirectory();
    std::string const simulations = data + "Simulations/";
    std::string const base = fileBasename(name);

    if (fileExists(simulations + base))
        return simulations + base;
    if (fileExists(data + name))
        return data + name;
    if (fileExists(simulations + name))
        return simulations + name;
    return {};
}

std::string siblingWithExtension(std::string const& path, char const* ext)
{
    return fileDirectory(path) + fileStem(path) + ext;
}

int64_t modificationTime(std::string const& path)
{
    struct stat info;
    if (path.empty() || (::stat(path.c_str(), &info) != 0))
        return 0;
    return int64_t(info.st_mtime);
}

bool readAll(std::string const& path, std::string& out)
{
    std::ifstream file(path);
    if (!file)
        return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

bool writeAll(std::string const& path, std::string const& text)
{
    std::ofstream file(path);
    if (!file)
        return false;
    file << text;
    return bool(file);
}

bool typeExists(Script const& script, std::string const& name)
{
    try { script.getPathType(name); return true; } catch (...) {}
    try { script.getWayType(name); return true; } catch (...) {}
    try { script.getUnitType(name); return true; } catch (...) {}
    try { script.getAreaType(name); return true; } catch (...) {}
    try { script.getAgentType(name); return true; } catch (...) {}
    try { script.getMapType(name); return true; } catch (...) {}
    return false;
}

std::vector<std::string> placedTypes(Simulation const& simulation)
{
    std::vector<std::string> names;
    auto const add = [&](std::string const& name) {
        if (name.empty())
            return;
        for (std::string const& existing: names)
        {
            if (existing == name)
                return;
        }
        names.push_back(name);
    };

    for (auto const& cityIt: simulation.cities())
    {
        City const& city = *cityIt.second;
        for (auto const& pathIt: city.paths())
        {
            add(pathIt.second->type());
            for (auto const& way: pathIt.second->ways())
                add(way->type());
        }
        for (auto const& unit: city.units())
            add(unit->type());
        for (auto const& area: city.areas())
            add(area->type());
        for (auto const& agent: city.agents())
            add(agent->type());
    }
    return names;
}

} // namespace

// ----------------------------------------------------------------------------
GlassBoxApp::GlassBoxApp(Options options)
    : host::Application(options.width, options.height, "OpenGlassBox"),
      m_options(std::move(options))
{}

// ----------------------------------------------------------------------------
GlassBoxApp::~GlassBoxApp()
{
    m_trace.setRecording(false);
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::onSetup()
{
    theme::loadFonts();
    ImPlot::CreateContext();
    imgui().setClearColor(0.05f, 0.06f, 0.08f);
    imgui().setDefaultLayoutCallback(
        [this](ImGuiID dockspace) { this->buildDefaultLayout(dockspace); });

    if (!loadPath(m_options.file))
    {
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
    ImGuiID center = dockspace;
    ImGuiID left = 0u;
    ImGuiID right = 0u;
    ImGuiID bottom = 0u;
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20f, &left, &center);
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, &right, &center);
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.32f, &bottom, &center);

    ImGuiID rightTop = 0u;
    ImGuiID rightBottom = 0u;
    ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.50f, &rightBottom, &rightTop);

    ImGui::DockBuilderDockWindow("Map", center);
    ImGui::DockBuilderDockWindow("Simulation clock", left);
    ImGui::DockBuilderDockWindow("Inspector", rightTop);
    ImGui::DockBuilderDockWindow("Traffic", rightBottom);
    ImGui::DockBuilderDockWindow("History", rightBottom);
    ImGui::DockBuilderDockWindow("Rule Log", bottom);
    ImGui::DockBuilderDockWindow("Charts", bottom);
    ImGui::DockBuilderDockWindow("Script", bottom);
}

// ----------------------------------------------------------------------------
void GlassBoxApp::createEmptyCity(Simulation& simulation, std::string const& name)
{
    City& city = simulation.addCity(name, Vector3f(0.0f, 0.0f, 0.0f),
                                    DEFAULT_CITY_SIZE, DEFAULT_CITY_SIZE);
    for (auto const& it: simulation.script().mapTypes())
        city.addMap(*it.second);

    // The city starts with no road, but it has to start with the graphs the
    // ruleset declares: an empty Path is what the road tool lays segments into.
    for (auto const& it: simulation.script().pathTypes())
        city.addPath(*it.second);
}

// ----------------------------------------------------------------------------
void GlassBoxApp::resetView()
{
    m_state.selection.clear();
    m_charts.clear();
    m_trace.clear();
    m_editor.reset();

    // Six maps drawn on top of each other is a mush in which none can be read,
    // so a fresh simulation shows the first one and lists the others in the
    // Maps tool, one click away.
    if (m_simulation && !m_simulation->cities().empty())
    {
        auto const& maps = m_simulation->cities().begin()->second->maps();
        if (m_state.primaryLayer.empty() && !maps.empty())
            m_state.primaryLayer = maps.begin()->second->type();

        m_state.soloLayer.clear();
        for (auto const& it: maps)
        {
            m_state.layer(it.first).visible =
                (it.first == m_state.primaryLayer);
        }
    }

    m_viewer.requestFrameAll();
}

// ----------------------------------------------------------------------------
void GlassBoxApp::loadScriptText()
{
    m_script_text.clear();
    if (!m_ruleset_path.empty())
        readAll(m_ruleset_path, m_script_text);
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::loadRuleset(std::string const& filename, bool loadSiblingSave)
{
    std::string const path = resolveDataFile(filename);
    if (path.empty())
    {
        m_script_error = "Cannot find '" + filename + "'";
        return false;
    }

    auto simulation = std::make_unique<Simulation>(DEFAULT_CITY_SIZE,
                                                   DEFAULT_CITY_SIZE);
    try
    {
        if (!simulation->script().parse(path))
        {
            m_script_error = "Failed parsing '" + path + "'";
            if (!simulation->script().formatErrors().empty())
                m_script_error += "\n" + simulation->script().formatErrors();
            return false;
        }
    }
    catch (std::exception const& e)
    {
        m_script_error = "Failed parsing '" + path + "': " + e.what();
        return false;
    }

    m_simulation = std::move(simulation);
    m_ruleset_path = path;
    m_save_path.clear();
    m_script_error.clear();
    loadScriptText();

    std::string const sibling = siblingWithExtension(path, ".ogc");
    if (loadSiblingSave && fileExists(sibling))
    {
        CitySaveHeader header;
        std::string error;
        if (!CitySave::peekHeader(sibling, header, error) ||
            !CitySave::matchesRuleset(header, path, error) ||
            !CitySave::read(sibling, *m_simulation, error))
        {
            m_script_error = error;
            createEmptyCity(*m_simulation, fileStem(path));
        }
        else
        {
            m_save_path = sibling;
        }
    }
    else
    {
        createEmptyCity(*m_simulation, fileStem(path));
    }

    resetView();
    m_script_mtime = modificationTime(m_ruleset_path);
    setTitle("OpenGlassBox - " + fileBasename(m_ruleset_path));
    return m_script_error.empty();
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::loadCity(std::string const& filename)
{
    std::string const path = resolveDataFile(filename);
    if (path.empty())
    {
        m_script_error = "Cannot find '" + filename + "'";
        return false;
    }

    CitySaveHeader header;
    std::string error;
    if (!CitySave::peekHeader(path, header, error))
    {
        m_script_error = error;
        return false;
    }

    std::string ruleset = joinPath(fileDirectory(path), header.ruleset);
    if (!fileExists(ruleset))
        ruleset = resolveDataFile(header.ruleset);
    if (ruleset.empty() || !fileExists(ruleset))
    {
        m_script_error = "This save needs ruleset '" + header.ruleset +
                         "'. Open it with File → Open ruleset.";
        return false;
    }

    if (!CitySave::matchesRuleset(header, ruleset, error))
    {
        m_script_error = error;
        return false;
    }

    auto simulation = std::make_unique<Simulation>(DEFAULT_CITY_SIZE,
                                                   DEFAULT_CITY_SIZE);
    try
    {
        if (!simulation->script().parse(ruleset))
        {
            m_script_error = "Failed parsing '" + ruleset + "'";
            return false;
        }
    }
    catch (std::exception const& e)
    {
        m_script_error = "Failed parsing '" + ruleset + "': " + e.what();
        return false;
    }

    if (!CitySave::read(path, *simulation, error))
    {
        m_script_error = error;
        return false;
    }

    m_simulation = std::move(simulation);
    m_ruleset_path = ruleset;
    m_save_path = path;
    m_script_error.clear();
    loadScriptText();
    resetView();
    m_script_mtime = modificationTime(m_ruleset_path);
    setTitle("OpenGlassBox - " + fileBasename(m_save_path));
    return true;
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::loadPath(std::string const& filename)
{
    std::string const path = resolveDataFile(filename);
    std::string const ext = fileExtension(path.empty() ? filename : path);
    if (ext == ".ogc")
        return loadCity(filename);
    return loadRuleset(filename, true);
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::saveCity(std::string const& filename)
{
    if (!m_simulation || m_ruleset_path.empty())
    {
        m_script_error = "Nothing to save: open a ruleset first.";
        return false;
    }

    std::string error;
    if (!CitySave::write(filename, *m_simulation, m_ruleset_path, error))
    {
        m_script_error = error;
        return false;
    }

    m_save_path = filename;
    m_reload_notice = "city saved";
    m_reload_notice_timer = NOTICE_DURATION;
    setTitle("OpenGlassBox - " + fileBasename(m_save_path));
    return true;
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::applyScript()
{
    if (!m_simulation || m_ruleset_path.empty())
        return false;

    auto next = std::make_unique<Simulation>(DEFAULT_CITY_SIZE, DEFAULT_CITY_SIZE);
    if (!next->script().parseString(m_script_text, m_ruleset_path))
    {
        m_script_error = "Apply failed:\n" + next->script().formatErrors();
        return false;
    }

    std::vector<std::string> missing;
    for (std::string const& type: placedTypes(*m_simulation))
    {
        if (!typeExists(next->script(), type))
            missing.push_back(type);
    }
    if (!missing.empty())
    {
        m_script_error = "Apply refused: the city still uses types the new "
                         "script dropped:";
        for (std::string const& type: missing)
            m_script_error += "\n  - " + type;
        return false;
    }

    if (!writeAll(m_ruleset_path, m_script_text))
    {
        m_script_error = "Cannot write '" + m_ruleset_path + "'";
        return false;
    }

    std::string const tmp = siblingWithExtension(m_ruleset_path, ".ogc.apply.tmp");
    std::string error;
    bool const hadCity = !m_simulation->cities().empty();
    if (hadCity && !CitySave::write(tmp, *m_simulation, m_ruleset_path, error))
    {
        m_script_error = error;
        return false;
    }

    if (hadCity)
    {
        if (!CitySave::read(tmp, *next, error))
        {
            m_script_error = error;
            std::remove(tmp.c_str());
            return false;
        }
        std::remove(tmp.c_str());
    }
    else
    {
        createEmptyCity(*next, fileStem(m_ruleset_path));
    }

    m_simulation = std::move(next);
    m_script_error.clear();
    m_script_status = "applied";
    m_script_mtime = modificationTime(m_ruleset_path);
    m_editor.reset();
    m_state.selection.clear();
    return true;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::watchScriptFile(float dt)
{
    if (!m_auto_reload || m_ruleset_path.empty())
        return;

    m_watch_timer -= dt;
    if (m_watch_timer > 0.0f)
        return;

    m_watch_timer = WATCH_PERIOD;

    int64_t const mtime = modificationTime(m_ruleset_path);
    if ((mtime == 0) || (mtime == m_script_mtime))
        return;

    loadScriptText();
    if (applyScript())
    {
        m_reload_notice = "ruleset reloaded";
        m_reload_notice_timer = NOTICE_DURATION;
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::openRulesetDialog()
{
    host::FileDialogRequest request;
    request.key = OPEN_RULESET_DIALOG;
    request.title = "Open a ruleset";
    request.filters = ".ogs,.txt,.*";
    request.startPath = dataDirectory() + "Simulations";
    request.onAccepted = [this](std::string const& path) {
        loadRuleset(path, false);
    };
    imgui().requestFileDialog(std::move(request));
}

// ----------------------------------------------------------------------------
void GlassBoxApp::openCityDialog()
{
    host::FileDialogRequest request;
    request.key = OPEN_CITY_DIALOG;
    request.title = "Open a city save";
    request.filters = ".ogc,.*";
    request.startPath = dataDirectory() + "Simulations";
    request.onAccepted = [this](std::string const& path) {
        loadCity(path);
    };
    imgui().requestFileDialog(std::move(request));
}

// ----------------------------------------------------------------------------
void GlassBoxApp::saveCityDialog()
{
    host::FileDialogRequest request;
    request.key = SAVE_CITY_DIALOG;
    request.title = "Save the city";
    request.filters = ".ogc,.*";
    request.startPath = m_save_path.empty()
                            ? dataDirectory() + "Simulations"
                            : m_save_path;
    request.onAccepted = [this](std::string const& path) {
        saveCity(path);
    };
    imgui().requestFileDialog(std::move(request));
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

    m_trace.setTick(m_simulation->totalTicks());
    m_simulation->update(dt);

    m_ticks_last_frame = m_simulation->totalTicks() - before;
    if (m_ticks_last_frame != 0u)
        m_charts.sample(*m_simulation);
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onUpdate(float dt)
{
    watchScriptFile(dt);
    advanceSimulation(dt);

    if (m_reload_notice_timer > 0.0f)
        m_reload_notice_timer -= dt;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onDrawMenuBar()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New city...", "Ctrl+N"))
            openRulesetDialog();
        if (ImGui::MenuItem("Open ruleset...", "Ctrl+Shift+O"))
            openRulesetDialog();
        if (ImGui::MenuItem("Open city...", "Ctrl+O"))
            openCityDialog();
        if (ImGui::MenuItem("Save city...", "Ctrl+S", false,
                            m_simulation != nullptr))
            saveCityDialog();

        ImGui::MenuItem("Reload ruleset when the file changes", nullptr,
                        &m_auto_reload);
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Alt+F4"))
            halt();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        editor::CommandStack& stack = m_editor.stack();

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

        struct Entry { editor::EditTool tool; char const* label; char const* shortcut; };
        static Entry const TOOLS[] = {
            { editor::EditTool::Select, "Inspect", "1" },
            { editor::EditTool::Road, "Roads", "2" },
            { editor::EditTool::Zone, "Zones", "3" },
            { editor::EditTool::Building, "Buildings", "4" },
            { editor::EditTool::Paint, "Maps", "5" },
            { editor::EditTool::Bulldozer, "Demolish", "6" },
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
        ImGui::MenuItem("Inspector", nullptr, &m_show_inspector);
        ImGui::MenuItem("Rule Log", nullptr, &m_show_rule_log);
        ImGui::MenuItem("Charts", nullptr, &m_show_charts);
        ImGui::MenuItem("Script", nullptr, &m_show_script);
        ImGui::MenuItem("Simulation clock", nullptr, &m_show_time);
        ImGui::MenuItem("Traffic", nullptr, &m_show_traffic);
        ImGui::MenuItem("History", nullptr, &m_show_history);
        ImGui::Separator();
        if (ImGui::MenuItem("Recenter the map", "Home"))
            m_viewer.requestFrameAll();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About"))
            m_show_about = true;
        ImGui::EndMenu();
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::onDrawPanels()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O, false))
        openRulesetDialog();
    else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
        openCityDialog();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
        openRulesetDialog();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && m_simulation)
        saveCityDialog();
    if (ImGui::IsKeyPressed(ImGuiKey_F5, false) && !m_ruleset_path.empty())
        applyScript();
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false))
        m_viewer.requestFrameAll();
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !io.WantTextInput &&
        m_simulation)
    {
        m_simulation->setPaused(!m_simulation->paused());
    }

    if (!io.WantTextInput && !io.KeyCtrl)
    {
        static editor::EditTool const SHORTCUTS[] = {
            editor::EditTool::Select, editor::EditTool::Road, editor::EditTool::Zone,
            editor::EditTool::Building, editor::EditTool::Paint,
            editor::EditTool::Bulldozer,
        };
        for (int i = 0; i < IM_ARRAYSIZE(SHORTCUTS); ++i)
        {
            if (ImGui::IsKeyPressed(ImGuiKey(int(ImGuiKey_1) + i), false))
                m_editor.setTool(SHORTCUTS[i]);
        }
    }

    if (m_simulation)
    {
        m_viewer.draw(*m_simulation, m_state, m_editor);

        if (m_show_history)
            m_editor.drawHistoryPanel(*m_simulation);
        if (m_show_time)
            m_time.draw(*m_simulation);
        if (m_show_inspector)
            m_inspector.draw(*m_simulation, m_state, m_trace);
        if (m_show_rule_log)
            m_rule_log.draw(*m_simulation, m_state, m_trace);
        if (m_show_charts)
            m_charts.draw(*m_simulation, m_state);
        if (m_show_traffic)
            m_traffic.draw(*m_simulation, m_state);
        if (m_show_script)
        {
            bool apply = false;
            m_script_panel.draw(m_script_text, apply, m_script_status);
            if (apply)
                applyScript();
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
                           "The operation was refused.");
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
        ImGui::TextUnformatted("One window is one city. The ruleset is a .ogs;\n"
                               "the save is a .ogc (geometry and live state).");
        ImGui::Separator();
        ImGui::BulletText("left: inspect / use the armed tool");
        ImGui::BulletText("middle or right drag: pan");
        ImGui::BulletText("wheel: zoom");
        ImGui::BulletText("space: pause");
        ImGui::BulletText("1-6: inspect, roads, zones, buildings, maps, demolish");
        ImGui::BulletText("Ctrl+Z / Ctrl+Y: undo / redo");
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
        ImGui::TextDisabled("no city loaded");
        return;
    }

    ImGui::TextColored(
        ImGui::ColorConvertU32ToFloat4(
            m_simulation->paused() ? theme::FAILURE : theme::SUCCESS),
        "%s", m_simulation->paused() ? "paused" : "running");

    ImGui::SameLine(0.0f, 20.0f);
    SimulationClock const& clock = m_simulation->clock();
    ImGui::Text("Jour %u  %02u:%02u  tick %llu x%.2f",
                clock.day(), clock.hourOfDay(), clock.minuteOfHour(),
                (unsigned long long)m_simulation->totalTicks(),
                m_simulation->timeScale());

    size_t units = 0u;
    size_t agents = 0u;
    for (auto const& it: m_simulation->cities())
    {
        units += it.second->units().size();
        agents += it.second->agents().size();
    }

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::Text("%zu unit(s), %zu agent(s)", units, agents);

    if (m_reload_notice_timer > 0.0f)
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::SUCCESS), "%s",
                           m_reload_notice.c_str());
    }

    if (!m_save_path.empty())
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextDisabled("%s", m_save_path.c_str());
    }
    else if (!m_ruleset_path.empty())
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextDisabled("%s", m_ruleset_path.c_str());
    }
}
} // namespace game
} // namespace ogb
