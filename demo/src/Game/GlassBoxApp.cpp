//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/DijkstraRouter.hpp"

#include "Game/GlassBoxApp.hpp"
#include "Save/CitySave.hpp"
#include "UI/Theme.hpp"

#include <implot.h>

#include <sys/stat.h>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "project_info.hpp"

namespace ogb
{
namespace game
{
using namespace ogb::theme;

static constexpr char const* OPEN_RULESET_DIALOG = "OpenRulesetDialog";
static constexpr char const* OPEN_CITY_DIALOG = "OpenCityDialog";
static constexpr char const* SAVE_CITY_DIALOG = "SaveCityDialog";
static constexpr float WATCH_PERIOD = 0.5f;
static constexpr float NOTICE_DURATION = 4.0f;
static constexpr uint32_t DEFAULT_CITY_SIZE = 16u;

namespace
{

ogb::Config defaultConfig()
{
    ogb::Config config;
    config.grid.defaultCitySizeU = DEFAULT_CITY_SIZE;
    config.grid.defaultCitySizeV = DEFAULT_CITY_SIZE;
    return config;
}

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
    return (slash == std::string::npos) ? std::string()
                                        : path.substr(0u, slash + 1u);
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

//! \brief Every directory a data file may sit in.
//!
//! project::info::paths::data is a colon separated search path, not a single
//! directory: reading it as one is why the demo could not find its own
//! simulations when started from the repository. None of its entries names the
//! sources either, where the data of the demo lives under demo/.
std::vector<std::string> dataDirectories()
{
    std::vector<std::string> directories;

    auto push = [&directories](std::string dir)
    {
        if (dir.empty())
            return;
        if ((dir.back() != '/') && (dir.back() != '\\'))
            dir += '/';
        directories.push_back(std::move(dir));
    };

    std::string const search = project::info::paths::data;
    size_t start = 0u;
    while (start <= search.size())
    {
        size_t const end = search.find(':', start);
        push(search.substr(start,
                           (end == std::string::npos) ? end : end - start));
        if (end == std::string::npos)
            break;
        start = end + 1u;
    }

    push("demo/data");
    push("../demo/data");
    return directories;
}

//! \brief The data directory that actually holds the simulations, so that the
//! file dialogs open where the files are.
std::string dataDirectory()
{
    std::vector<std::string> const directories = dataDirectories();
    for (std::string const& dir : directories)
    {
        if (fileExists(dir + "Simulations"))
            return dir;
    }
    return directories.empty() ? std::string() : directories.front();
}

//! \brief As given if the path exists, otherwise looked up by name in the data
//! directories and in their simulations/ subdirectory.
std::string resolveDataFile(std::string const& name)
{
    if (fileExists(name))
        return name;

    std::string const base = fileBasename(name);
    for (std::string const& data : dataDirectories())
    {
        std::string const simulations = data + "simulations/";
        if (fileExists(simulations + base))
            return simulations + base;
        if (fileExists(data + name))
            return data + name;
        if (fileExists(simulations + name))
            return simulations + name;
    }
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

bool typeExists(Ruleset const& script, std::string const& name)
{
    try
    {
        script.getPathType(name);
        return true;
    }
    catch (...)
    {
    }
    try
    {
        script.getSegmentType(name);
        return true;
    }
    catch (...)
    {
    }
    try
    {
        script.getBuildingType(name);
        return true;
    }
    catch (...)
    {
    }
    try
    {
        script.getZoneType(name);
        return true;
    }
    catch (...)
    {
    }
    try
    {
        script.getAgentType(name);
        return true;
    }
    catch (...)
    {
    }
    try
    {
        script.getLayerType(name);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

std::vector<std::string> placedTypes(Simulation const& simulation)
{
    std::vector<std::string> names;
    auto const add = [&](Name const& name)
    {
        if (name.empty())
            return;
        for (std::string const& existing : names)
        {
            if (existing == name.str())
                return;
        }
        names.push_back(name.str());
    };

    for (auto const& cityIt : simulation.getCities())
    {
        City const& city = *cityIt.second;
        for (auto const& pathIt : city.getPaths())
        {
            add(pathIt.second->getTypeName());
            for (auto const& segment : pathIt.second->getSegments())
                add(segment->getTypeName());
        }
        for (auto const& building : city.getBuildings())
            add(building->getTypeName());
        for (auto const& zone : city.getZones())
            add(zone->getTypeName());
        for (auto const& agent : city.getAgents())
            add(agent->getTypeName());
    }
    return names;
}

} // namespace

// ----------------------------------------------------------------------------
GlassBoxApp::GlassBoxApp(Options options)
    : host::Application(options.width, options.height, "OpenGlassBox"),
      m_options(std::move(options))
{
}

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
    imgui().setDefaultLayoutCallback([this](ImGuiID dockspace)
                                     { this->buildDefaultLayout(dockspace); });

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
    ImGui::DockBuilderSplitNode(
        right, ImGuiDir_Down, 0.50f, &rightBottom, &rightTop);

    ImGuiID leftTop = 0u;
    ImGuiID leftBottom = 0u;
    ImGui::DockBuilderSplitNode(
        left, ImGuiDir_Down, 0.60f, &leftBottom, &leftTop);

    ImGui::DockBuilderDockWindow("Layer", center);
    ImGui::DockBuilderDockWindow("Simulation clock", leftTop);
    ImGui::DockBuilderDockWindow("Layers", leftBottom);
    ImGui::DockBuilderDockWindow("Inspector", rightTop);
    ImGui::DockBuilderDockWindow("Traffic", rightBottom);
    ImGui::DockBuilderDockWindow("Budget", rightBottom);
    ImGui::DockBuilderDockWindow("History", rightBottom);
    ImGui::DockBuilderDockWindow("Rule Log", bottom);
    ImGui::DockBuilderDockWindow("Charts", bottom);
    ImGui::DockBuilderDockWindow("Script", bottom);
}

// ----------------------------------------------------------------------------
void GlassBoxApp::RouterListener::onCityAdded(City& city)
{
    installDijkstraRouter(city, city.getConfig());
}

// ----------------------------------------------------------------------------
void GlassBoxApp::wireSimulation(Simulation& simulation)
{
    simulation.setListener(m_router_listener);
}

// ----------------------------------------------------------------------------
void GlassBoxApp::createEmptyCity(Simulation& simulation,
                                  std::string const& name) const
{
    City& city = simulation.addCity(
        name, Vector3f(0.0f, 0.0f, 0.0f), DEFAULT_CITY_SIZE, DEFAULT_CITY_SIZE);
    for (auto const& it : simulation.getRuleset().getLayerTypes())
        city.addLayer(*it.second);

    // The city starts with no road, but it has to start with the graphs the
    // ruleset declares: an empty Path is what the road tool lays segments into.
    for (auto const& it : simulation.getRuleset().getPathTypes())
        city.addPath(*it.second);
}

// ----------------------------------------------------------------------------
void GlassBoxApp::resetView()
{
    m_state.selection.clear();
    m_charts.clear();
    m_trace.clear();
    m_editor.reset();

    // Six layers drawn on top of each other is a mush in which none can be
    // read, so a fresh simulation shows the first one and lists the others in
    // the Layers tool, one click away.
    if (m_simulation && !m_simulation->getCities().empty())
    {
        auto const& layers =
            m_simulation->getCities().begin()->second->getLayers();
        if (m_state.primaryLayer.empty() && !layers.empty())
            m_state.primaryLayer = layers.begin()->second->getTypeName().str();

        m_state.soloLayer.clear();
        for (auto const& it : layers)
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

    auto simulation = std::make_unique<Simulation>(defaultConfig());
    wireSimulation(*simulation);
    try
    {
        if (!simulation->loadScriptFile(path))
        {
            m_script_error = "Failed parsing '" + path + "'";
            if (!simulation->getRuleset().formatErrors().empty())
                m_script_error +=
                    "\n" + simulation->getRuleset().formatErrors();
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
            !acceptRuleset(header, path, error) ||
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
    computeChecksum();
    setTitle("OpenGlassBox - " + fileBasename(m_ruleset_path));
    return m_script_error.empty();
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::acceptRuleset(CitySaveHeader const& header,
                                std::string const& rulesetPath,
                                std::string& error)
{
    if (CitySave::matchesRuleset(header, rulesetPath, error))
        return true;

    if (!m_script_options.ignoreMismatch)
        return false;

    // The geometry is still read against the types the header names, and a
    // type the new script dropped is still refused by name. What is waived is
    // only the promise that the rules did not move underneath the city.
    m_reload_notice = "checksum ignored: " + error;
    m_reload_notice_timer = NOTICE_DURATION;
    error.clear();
    return true;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::computeChecksum()
{
    listRulesets();

    m_checksum.known = true;
    m_checksum.onDisk = m_ruleset_path.empty()
                            ? std::string()
                            : CitySave::hashFile(m_ruleset_path);
    m_checksum.edited = CitySave::hashString(m_script_text);
    m_checksum.save.clear();
    m_checksum.staleSaves.clear();

    if (m_ruleset_path.empty())
        return;

    // The open save is usually one of those, but a city opened from elsewhere
    // is worth reporting too.
    std::vector<std::string> saves =
        CitySave::savesUsingRuleset(m_ruleset_path);
    if (!m_save_path.empty() &&
        (std::find(saves.begin(), saves.end(), m_save_path) == saves.end()))
    {
        saves.push_back(m_save_path);
    }

    for (std::string const& save : saves)
    {
        CitySaveHeader header;
        std::string error;
        if (!CitySave::peekHeader(save, header, error))
            continue;

        if (save == m_save_path)
            m_checksum.save = header.hash;
        if (header.hash != m_checksum.onDisk)
            m_checksum.staleSaves.push_back(save);
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::listRulesets()
{
    namespace fs = std::filesystem;

    m_ruleset_files.current = m_ruleset_path;
    m_ruleset_files.rulesets.clear();
    if (m_ruleset_path.empty())
        return;

    std::error_code code;
    fs::path const ruleset(m_ruleset_path);
    fs::path const directory =
        ruleset.has_parent_path() ? ruleset.parent_path() : fs::path(".");

    for (auto const& entry : fs::directory_iterator(directory, code))
    {
        if (entry.is_regular_file(code) && (entry.path().extension() == ".ogs"))
            m_ruleset_files.rulesets.push_back(entry.path().string());
    }

    // A directory hands its entries out in whatever order it pleases, and a
    // list that shuffles between runs is a list nobody can point at.
    std::sort(m_ruleset_files.rulesets.begin(), m_ruleset_files.rulesets.end());
}

// ----------------------------------------------------------------------------
void GlassBoxApp::switchRuleset(std::string const& path)
{
    if (path.empty() || (path == m_ruleset_path))
        return;

    // Opening a ruleset founds an empty city, so whatever is on screen is
    // about to be dropped. Asking is only worth it when there is something to
    // lose: a city that was never built on is not worth a modal.
    bool built = false;
    if (m_simulation)
    {
        for (auto const& it : m_simulation->getCities())
        {
            City const& city = *it.second;
            built = built || !city.getBuildings().empty() ||
                    !city.getZones().empty();
            for (auto const& pathIt : city.getPaths())
                built = built || !pathIt.second->getSegments().empty();
        }
    }

    if (!built)
    {
        loadRuleset(path, false);
        return;
    }

    m_pending_ruleset = path;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::drawSwitchRulesetPopup()
{
    if (m_pending_ruleset.empty())
        return;

    static constexpr char const* TITLE = "Open another ruleset?";
    if (!ImGui::IsPopupOpen(TITLE))
        ImGui::OpenPopup(TITLE);

    if (!ImGui::BeginPopupModal(
            TITLE, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::Text("Open '%s'?", fileBasename(m_pending_ruleset).c_str());
    ImGui::TextDisabled(
        "The city on screen is replaced by an empty one. Anything\n"
        "not saved is lost.");
    ImGui::Separator();

    if (ImGui::Button("Open"))
    {
        std::string const path = m_pending_ruleset;
        m_pending_ruleset.clear();
        ImGui::CloseCurrentPopup();
        loadRuleset(path, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        m_pending_ruleset.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

// ----------------------------------------------------------------------------
size_t GlassBoxApp::restampStaleSaves()
{
    size_t stamped = 0u;
    for (std::string const& save : m_checksum.staleSaves)
    {
        std::string error;
        if (CitySave::restamp(save, m_ruleset_path, error))
            ++stamped;
        else
            m_script_error = error;
    }

    computeChecksum();
    return stamped;
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

    if (!acceptRuleset(header, ruleset, error))
    {
        m_script_error = error;
        return false;
    }

    auto simulation = std::make_unique<Simulation>(defaultConfig());
    wireSimulation(*simulation);
    try
    {
        if (!simulation->loadScriptFile(ruleset))
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
    computeChecksum();
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
    computeChecksum();
    setTitle("OpenGlassBox - " + fileBasename(m_save_path));
    return true;
}

// ----------------------------------------------------------------------------
bool GlassBoxApp::applyScript()
{
    if (!m_simulation || m_ruleset_path.empty())
        return false;

    auto next = std::make_unique<Simulation>(defaultConfig());
    wireSimulation(*next);
    if (!next->loadScriptString(m_script_text, m_ruleset_path))
    {
        m_script_error = "Apply failed:\n" + next->getRuleset().formatErrors();
        return false;
    }

    std::vector<std::string> missing;
    for (std::string const& type : placedTypes(*m_simulation))
    {
        if (!typeExists(next->getRuleset(), type))
            missing.push_back(type);
    }
    if (!missing.empty())
    {
        m_script_error = "Apply refused: the city still uses types the new "
                         "script dropped:";
        for (std::string const& type : missing)
            m_script_error += "\n  - " + type;
        return false;
    }

    if (!writeAll(m_ruleset_path, m_script_text))
    {
        m_script_error = "Cannot write '" + m_ruleset_path + "'";
        return false;
    }

    std::string const tmp =
        siblingWithExtension(m_ruleset_path, ".ogc.apply.tmp");
    std::string error;
    bool const hadCity = !m_simulation->getCities().empty();
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

    // Rewriting the ruleset just invalidated every save beside it, and a save
    // that no longer opens is the whole of what the fingerprint costs while a
    // script is being written. The saves name this ruleset, so they can be
    // found and stamped rather than left for the player to mend one by one.
    computeChecksum();
    size_t const stamped = restampStaleSaves();
    if (stamped != 0u)
    {
        m_script_status =
            "applied, " + std::to_string(stamped) + " save(s) stamped";
    }
    return true;
}

// ----------------------------------------------------------------------------
void GlassBoxApp::watchScriptFile(float dt)
{
    if (!m_script_options.autoReload || m_ruleset_path.empty())
        return;

    m_watch_timer -= dt;
    if (m_watch_timer > 0.0f)
        return;

    m_watch_timer = WATCH_PERIOD;

    int64_t const mtime = modificationTime(m_ruleset_path);
    if ((mtime == 0) || (mtime == m_script_mtime))
        return;

    // Remember the file as seen even when what it holds is refused: otherwise
    // the same broken save is reparsed twice a second and its error popup can
    // never be dismissed.
    m_script_mtime = mtime;

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
    request.onAccepted = [this](std::string const& path)
    { loadRuleset(path, false); };
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
    request.onAccepted = [this](std::string const& path) { loadCity(path); };
    imgui().requestFileDialog(std::move(request));
}

// ----------------------------------------------------------------------------
void GlassBoxApp::saveCityDialog()
{
    host::FileDialogRequest request;
    request.key = SAVE_CITY_DIALOG;
    request.title = "Save the city";
    request.filters = ".ogc,.*";
    request.startPath =
        m_save_path.empty() ? dataDirectory() + "Simulations" : m_save_path;
    request.onAccepted = [this](std::string const& path) { saveCity(path); };
    imgui().requestFileDialog(std::move(request));
}

// ----------------------------------------------------------------------------
void GlassBoxApp::advanceSimulation(float dt)
{
    if (!m_simulation)
        return;

    uint64_t const before = m_simulation->getClock().getTicks();

    uint32_t steps = m_time.takePendingSteps();
    while (steps-- > 0u)
    {
        m_trace.setTick(m_simulation->getClock().getTicks());
        m_simulation->stepOneTick();
        m_charts.sample(*m_simulation);
    }

    m_trace.setTick(m_simulation->getClock().getTicks());
    m_simulation->update(dt);

    m_ticks_last_frame = m_simulation->getClock().getTicks() - before;
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
        if (ImGui::MenuItem(
                "Save city...", "Ctrl+S", false, m_simulation != nullptr))
            saveCityDialog();

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
                            "Ctrl+Z",
                            false,
                            stack.canUndo()) &&
            m_simulation)
        {
            m_editor.undo(*m_simulation);
        }
        if (ImGui::MenuItem(stack.canRedo()
                                ? ("Redo " + stack.redoLabel()).c_str()
                                : "Redo",
                            "Ctrl+Y",
                            false,
                            stack.canRedo()) &&
            m_simulation)
        {
            m_editor.redo(*m_simulation);
        }

        ImGui::Separator();

        struct Entry
        {
            editor::EditTool tool;
            char const* label;
            char const* shortcut;
        };
        static Entry const TOOLS[] = {
            { editor::EditTool::Select, "Inspect", "1" },
            { editor::EditTool::Road, "Roads", "2" },
            { editor::EditTool::Node, "Nodes", "3" },
            { editor::EditTool::Zone, "Zones", "4" },
            { editor::EditTool::Building, "Buildings", "5" },
            { editor::EditTool::Paint, "Layers", "6" },
            { editor::EditTool::Bulldozer, "Demolish", "7" },
        };
        for (auto const& entry : TOOLS)
        {
            if (ImGui::MenuItem(
                    entry.label, entry.shortcut, m_editor.tool() == entry.tool))
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
        ImGui::MenuItem("Script", nullptr, &m_show_script);
        ImGui::MenuItem("Simulation clock", nullptr, &m_show_time);
        ImGui::MenuItem("Budget", nullptr, &m_show_budget);
        ImGui::MenuItem("Traffic", nullptr, &m_show_traffic);
        ImGui::MenuItem("History", nullptr, &m_show_history);
        ImGui::Separator();
        if (ImGui::MenuItem("Recenter the layer", "Home"))
            m_viewer.requestFrameAll();
        if (ImGui::MenuItem("Reset the panel layout"))
            imgui().resetLayout();
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
        m_simulation->setPaused(!m_simulation->isPaused());
    }

    if (!io.WantTextInput && !io.KeyCtrl)
    {
        static editor::EditTool const SHORTCUTS[] = {
            editor::EditTool::Select,    editor::EditTool::Road,
            editor::EditTool::Node,      editor::EditTool::Zone,
            editor::EditTool::Building,  editor::EditTool::Paint,
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

        // Arming a tool opens the panel it works with, since the toolbar no
        // longer carries the controls itself.
        switch (m_editor.takePanelRequest())
        {
            case editor::PanelRequest::Layers:
                m_show_layers = true;
                m_focus_panel = "Layers";
                break;
            case editor::PanelRequest::Inspector:
                m_show_inspector = true;
                m_focus_panel = "Inspector";
                break;
            case editor::PanelRequest::None:
                break;
        }

        if (m_show_layers)
            m_layers.draw(*m_simulation, m_state, m_show_layers);
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
        if (m_show_budget)
            m_budget.draw(*m_simulation);
        if (m_show_traffic)
            m_traffic.draw(*m_simulation, m_state);
        if (m_show_script)
            drawScriptPanel();
    }

    // Once the window has been submitted this frame, so that a panel just
    // reopened can be raised.
    if (m_focus_panel != nullptr)
    {
        ImGui::SetWindowFocus(m_focus_panel);
        m_focus_panel = nullptr;
    }

    drawScriptError();
    drawAboutPopup();
}

// ----------------------------------------------------------------------------
void GlassBoxApp::drawScriptPanel()
{
    // Only the text being typed is hashed every frame: it is in memory, while
    // the ruleset and the saves are on disk and only move when a file is read
    // or written.
    m_checksum.edited = CitySave::hashString(m_script_text);

    ui::ScriptPanel::Actions actions;
    m_script_panel.draw(*m_simulation,
                        m_script_text,
                        m_script_status,
                        m_checksum,
                        m_ruleset_files,
                        m_script_options,
                        actions);

    if (actions.apply)
    {
        applyScript();
    }
    if (actions.restampSaves)
    {
        size_t const stamped = restampStaleSaves();
        m_reload_notice = std::to_string(stamped) + " save(s) stamped";
        m_reload_notice_timer = NOTICE_DURATION;
    }
    if (!actions.openRuleset.empty())
    {
        switchRuleset(actions.openRuleset);
    }

    drawSwitchRulesetPopup();
}

// ----------------------------------------------------------------------------
void GlassBoxApp::drawScriptError()
{
    if (m_script_error.empty())
    {
        m_script_error_shown = false;
        return;
    }

    // Opened once, on the edge. Calling OpenPopup on every frame put the popup
    // straight back up after the player closed it, and an error raised again
    // by the file watcher on every attempt then pinned the whole application
    // behind a modal that could not be dismissed.
    if (!m_script_error_shown)
    {
        m_script_error_shown = true;
        ImVec2 const centre = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(
            centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup("Script error");
    }

    ImGui::SetNextWindowSize(ImVec2(600.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(
            "Script error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                           "The operation was refused.");
        ImGui::Separator();

        // A parse reports every error it found, which is a page of text on a
        // bad file. Left to grow, the popup ran past the bottom of the screen
        // and took the Close button with it.
        if (ImGui::BeginChild(
                "message", ImVec2(580.0f, 240.0f), ImGuiChildFlags_Borders))
        {
            ImGui::PushTextWrapPos(560.0f);
            ImGui::TextUnformatted(m_script_error.c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::EndChild();
        ImGui::Separator();

        bool close = ImGui::Button("Close", ImVec2(120.0f, 0.0f));
        ImGui::SameLine();
        if (ImGui::Button("Copy", ImVec2(120.0f, 0.0f)))
            ImGui::SetClipboardText(m_script_error.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("Escape also closes this.");

        close = close || ImGui::IsKeyPressed(ImGuiKey_Escape, false);
        if (close)
        {
            m_script_error.clear();
            m_script_error_shown = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    else if (!ImGui::IsPopupOpen("Script error"))
    {
        // Another popup took the level and closed this one. Let the message
        // go rather than hold an error nothing will ever show again.
        m_script_error.clear();
        m_script_error_shown = false;
    }
}

// ----------------------------------------------------------------------------
void GlassBoxApp::drawAboutPopup()
{
    if (!m_show_about)
        return;

    ImGui::OpenPopup("About OpenGlassBox");
    if (ImGui::BeginPopupModal(
            "About OpenGlassBox", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("OpenGlassBox");
        ImGui::TextDisabled("An open implementation of the GlassBox engine\n"
                            "of SimCity 2013.");
        ImGui::Separator();
        ImGui::TextUnformatted(
            "One window is one city. The ruleset is a .ogs;\n"
            "the save is a .ogc (geometry and live state).");
        ImGui::Separator();
        ImGui::BulletText("left: inspect / use the armed tool");
        ImGui::BulletText("middle or right drag: pan");
        ImGui::BulletText("wheel: zoom");
        ImGui::BulletText("space: pause");
        ImGui::BulletText("1-7: inspect, roads, nodes, zones, buildings,\n"
                          "     layers, demolish");
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

    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(m_simulation->isPaused()
                                                          ? theme::FAILURE
                                                          : theme::SUCCESS),
                       "%s",
                       m_simulation->isPaused() ? "paused" : "running");

    ImGui::SameLine(0.0f, 20.0f);
    SimulationClock const& clock = m_simulation->getClock();
    ImGui::Text("Day %u  %02u:%02u  tick %llu x%.2f",
                clock.getDay(),
                clock.getHourOfDay(),
                clock.getMinuteOfHour(),
                (unsigned long long)m_simulation->getClock().getTicks(),
                m_simulation->getTimeScale());

    size_t buildings = 0u;
    size_t agents = 0u;
    for (auto const& it : m_simulation->getCities())
    {
        buildings += it.second->getBuildings().size();
        agents += it.second->getAgents().size();
    }

    ImGui::SameLine(0.0f, 20.0f);
    ImGui::Text("%zu building(s), %zu agent(s)", buildings, agents);

    if (m_reload_notice_timer > 0.0f)
    {
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::SUCCESS),
                           "%s",
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
