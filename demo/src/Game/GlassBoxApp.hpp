//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file GlassBoxApp.hpp
//! \brief Main demo application wiring the simulation engine to the UI panels.

#ifndef OPEN_GLASSBOX_DEMO_GLASSBOX_APP_HPP
#define OPEN_GLASSBOX_DEMO_GLASSBOX_APP_HPP

#include "Editor/Editor.hpp"
#include "Game/DebugState.hpp"
#include "Game/RuleTrace.hpp"
#include "Host/Application.hpp"
#include "UI/CityViewer.hpp"
#include "UI/Panels.hpp"

#include "OpenGlassBox/Simulation.hpp"
#include "Save/CitySave.hpp"

#include <memory>
#include <string>

namespace ogb
{
namespace game
{

// ****************************************************************************
//! \brief The demo: owns the simulation, the debug panels and the wiring
//! between them. One window is one city.
// ****************************************************************************
class GlassBoxApp: public host::Application
{
public:

    struct Options
    {
        //! \brief Optional .ogs or .ogc, resolved against the simulations/
        //! directory.
        std::string file = "sandbox.ogs";
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
    void createEmptyCity(Simulation& simulation, std::string const& name) const;
    void loadScriptText();
    void openRulesetDialog();
    void openCityDialog();
    void saveCityDialog();
    void watchScriptFile(float dt);
    void drawScriptPanel();

    //! \brief Read the fingerprints the Script panel shows, and which of the
    //! saves beside the ruleset went stale. Hashing files and walking a
    //! directory is not free, so it is done when one of them is read or
    //! written rather than every frame.
    void computeChecksum();

    //! \brief List the rulesets sitting beside the open one, for the picker of
    //! the Script panel. Walked with the checksums, for the same reason.
    void listRulesets();

    //! \brief Open the ruleset the player picked in the Script panel, asking
    //! first when the city on screen holds something worth losing.
    void switchRuleset(std::string const& path);
    void drawSwitchRulesetPopup();

    //! \brief Stamp the current fingerprint of the ruleset into every save
    //! computeChecksum() found stale, and read the fingerprints again.
    //! \return how many saves were stamped.
    size_t restampStaleSaves();

    //! \brief Whether a save may be opened against the ruleset on disk. A
    //! mismatch is normally a hard refusal; the player can waive it while
    //! writing a ruleset, and then it is only reported.
    bool acceptRuleset(CitySaveHeader const& header,
                       std::string const& rulesetPath,
                       std::string& error);

    //! \brief Install the demo router listener before any city is founded.
    void wireSimulation(Simulation& simulation);

private:

    struct RouterListener: Simulation::Listener
    {
        void onCityAdded(City& city) override;
    };

    Options m_options;
    RouterListener m_router_listener;
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
    ui::BudgetPanel m_budget;
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
    //! \brief Whether the error popup is already up, so that an error raised
    //! again on the next frame cannot pin the application behind a modal.
    bool m_script_error_shown = false;

    ui::ScriptPanel::Checksum m_checksum;
    //! \brief The rulesets the Script panel offers to switch to.
    ui::ScriptPanel::Files m_ruleset_files;
    //! \brief Watching the file and waiving the fingerprint of a save. Both
    //! belong to the Script panel, which is where the fingerprint is read and
    //! where the file is edited.
    ui::ScriptPanel::Options m_script_options;
    //! \brief Ruleset picked in the Script panel and waiting for the player to
    //! confirm that the city on screen may be thrown away.
    std::string m_pending_ruleset;

    int64_t m_script_mtime = 0;
    float m_watch_timer = 0.0f;

    bool m_show_layers = true;
    bool m_show_inspector = true;
    bool m_show_rule_log = true;
    bool m_show_charts = true;
    bool m_show_time = true;
    bool m_show_budget = true;
    bool m_show_traffic = true;
    bool m_show_history = false;
    bool m_show_script = true;
    //! \brief Panel to raise on the next frame, once it has been submitted to
    //! ImGui: focusing a window that does not exist yet does nothing.
    char const* m_focus_panel = nullptr;

    uint64_t m_ticks_last_frame = 0u;
};
} // namespace game
} // namespace ogb

#endif
