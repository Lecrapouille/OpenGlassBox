//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file GlassBoxApp.hpp
//! \brief Main demo application wiring the simulation engine to the UI panels.


#ifndef OPEN_GLASSBOX_DEMO_GLASSBOX_APP_HPP
#  define OPEN_GLASSBOX_DEMO_GLASSBOX_APP_HPP

#  include "Host/Application.hpp"
#  include "Game/DebugState.hpp"
#  include "Editor/Editor.hpp"
#  include "Game/RuleTrace.hpp"
#  include "UI/CityViewer.hpp"
#  include "UI/Panels.hpp"

#  include "OpenGlassBox/Simulation.hpp"

#  include <memory>
#  include <string>

namespace ogb {
namespace game {


// ****************************************************************************
//! \brief The demo: owns the simulation, the debug panels and the wiring
//! between them. One window is one city.
// ****************************************************************************
class GlassBoxApp: public host::Application
{
public:

    struct Options
    {
        //! \brief Optional .ogs or .ogc, resolved against Simulations/.
        std::string file = "test_city.ogs";
        int width = 1600;
        int height = 900;
    };

    explicit GlassBoxApp(Options options);
    ~GlassBoxApp() override;

    bool loadPath(std::string const& filename);
    bool loadRuleset(std::string const& filename, bool loadSiblingSave);
    bool loadCity(std::string const& filename);
    bool saveCity(std::string const& filename);
    bool applyScript();

private:

    bool onSetup() override;
    void onTeardown() override;
    void onUpdate(float dt) override;
    void onDrawMenuBar() override;
    void onDrawPanels() override;
    void onDrawStatusBar() override;

    std::string imguiIniFilename() const override
    {
        return "openglassbox-layout.ini";
    }

    void buildDefaultLayout(ImGuiID dockspace);
    void advanceSimulation(float dt);
    void drawAboutPopup();
    void drawScriptError();
    void resetView();
    void createEmptyCity(Simulation& simulation, std::string const& name);
    void loadScriptText();
    void openRulesetDialog();
    void openCityDialog();
    void saveCityDialog();
    void watchScriptFile(float dt);

private:

    Options m_options;
    std::unique_ptr<Simulation> m_simulation;
    DebugState m_state;
    RuleTrace m_trace;

    ui::CityViewer m_viewer;
    editor::Editor m_editor;
    ui::LayersPanel m_layers;
    ui::InspectorPanel m_inspector;
    ui::RuleLogPanel m_rule_log;
    ui::ChartsPanel m_charts;
    ui::TimeControlPanel m_time;
    ui::TrafficPanel m_traffic;
    ui::ScriptPanel m_script_panel;

    std::string m_ruleset_path;
    std::string m_save_path;
    std::string m_script_text;
    std::string m_script_error;
    std::string m_script_status;
    std::string m_reload_notice;
    float m_reload_notice_timer = 0.0f;
    bool m_show_about = false;

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
    bool m_show_script = true;

    uint64_t m_ticks_last_frame = 0u;
};
} // namespace game
} // namespace ogb

#endif
