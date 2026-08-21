//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_DEMO_GLASSBOX_APP_HPP
#  define OPEN_GLASSBOX_DEMO_GLASSBOX_APP_HPP

#  include "Application/Application.hpp"
#  include "Core/DataPath.hpp"
#  include "Core/DebugState.hpp"
#  include "Core/Editor.hpp"
#  include "Core/RuleTrace.hpp"
#  include "UI/CityViewer.hpp"
#  include "UI/Panels.hpp"

#  include "OpenGlassBox/Simulation.hpp"

#  include <memory>
#  include <string>

namespace ogb {

// ****************************************************************************
//! \brief The demo: owns the simulation, the debug panels and the wiring
//! between them.
// ****************************************************************************
class GlassBoxApp: public Application
{
public:

    // ------------------------------------------------------------------------
    //! \brief Options gathered from the command line.
    // ------------------------------------------------------------------------
    struct Options
    {
        //! \brief Simulation script to load on startup, relative to the data
        //! path or absolute.
        std::string script = "Simulations/TestCity.txt";
        //! \brief Extra data directories, colon separated. Highest priority.
        std::string dataPath;
        int width = 1600;
        int height = 900;
    };

    explicit GlassBoxApp(Options options);
    ~GlassBoxApp() override;

    // ------------------------------------------------------------------------
    //! \brief Load a simulation script and rebuild the demo world from it.
    //! \return false when the script could not be parsed, in which case the
    //! previous world is left untouched.
    // ------------------------------------------------------------------------
    bool loadScript(std::string const& filename);

    // ------------------------------------------------------------------------
    //! \brief Reload the script currently loaded and put the edits back on top.
    //!
    //! The entities hold const references into the types the script owns, so
    //! the world cannot survive a reparse and is rebuilt. What can be kept is
    //! the work of the player: every edit went through a command, so replaying
    //! the history restores the roads laid and the buildings placed. What is
    //! lost is what the rules accumulated, which is the point of reloading.
    // ------------------------------------------------------------------------
    bool reloadScript();

private:

    // ------------------------------------------------------------------------
    //! \brief Derived from Application.
    // ------------------------------------------------------------------------
    bool onSetup() override;
    void onTeardown() override;
    void onUpdate(float dt) override;
    void onDrawMenuBar() override;
    void onDrawPanels() override;
    void onDrawStatusBar() override;

    // ------------------------------------------------------------------------
    //! \brief Keep the layout of the demo away from the imgui.ini that any
    //! other ImGui application of the working directory may have left.
    // ------------------------------------------------------------------------
    std::string imguiIniFilename() const override
    {
        return "openglassbox-layout.ini";
    }

    // ------------------------------------------------------------------------
    //! \brief Arrange the panels around the map on the very first run.
    // ------------------------------------------------------------------------
    void buildDefaultLayout(ImGuiID dockspace);

    // ------------------------------------------------------------------------
    //! \brief Build the sandbox world used to exercise the engine: two cities,
    //! their road networks and a handful of units.
    // ------------------------------------------------------------------------
    void buildDemoWorld();

    // ------------------------------------------------------------------------
    //! \brief Advance the simulation, sampling the charts once per tick so that
    //! the abscissa of the curves is the tick and not the frame.
    // ------------------------------------------------------------------------
    void advanceSimulation(float dt);

    void drawAboutPopup();
    void drawScriptError();

    // ------------------------------------------------------------------------
    //! \brief Ask the host for the script file picker.
    // ------------------------------------------------------------------------
    void openScriptDialog();

    // ------------------------------------------------------------------------
    //! \brief Reload the script when the file changed on disk, polling at most
    //! a few times a second. Editing a simulation script and seeing the result
    //! without leaving the editor is the whole point.
    // ------------------------------------------------------------------------
    void watchScriptFile(float dt);

private:

    Options m_options;
    DataPath m_data_path;
    //! \brief Rebuilt from scratch on every script load, hence the pointer.
    std::unique_ptr<Simulation> m_simulation;
    DebugState m_state;
    RuleTrace m_trace;

    CityViewer m_viewer;
    Editor m_editor;
    LayersPanel m_layers;
    InspectorPanel m_inspector;
    RuleLogPanel m_rule_log;
    ChartsPanel m_charts;
    TimeControlPanel m_time;
    TrafficPanel m_traffic;

    //! \brief Path of the script currently loaded, for the title and the reload.
    std::string m_script_path;
    //! \brief Message of the last failed load, shown in a modal.
    std::string m_script_error;
    //! \brief Message of the last successful reload, shown in the status bar
    //! for a few seconds so that a hot reload is not silent.
    std::string m_reload_notice;
    float m_reload_notice_timer = 0.0f;
    bool m_show_about = false;

    //! \brief Modification time of the script when it was loaded, and the
    //! countdown to the next check of it.
    int64_t m_script_mtime = 0;
    float m_watch_timer = 0.0f;
    bool m_auto_reload = true;

    bool m_show_layers = true;
    bool m_show_inspector = true;
    bool m_show_rule_log = true;
    bool m_show_charts = true;
    bool m_show_time = true;
    bool m_show_traffic = true;
    bool m_show_history = false;

    //! \brief Ticks run during the last frame, shown in the status bar.
    uint64_t m_ticks_last_frame = 0u;
};

} // namespace ogb

#endif
