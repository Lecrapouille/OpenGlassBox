//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//
// Adapted from the Oakular application layer of
// https://github.com/Lecrapouille/BlackThorn
//-----------------------------------------------------------------------------

#include "Application/DearImGuiApplication.hpp"
#include "UI/Theme.hpp"

#include <ImGuiFileDialog.h>

#include <algorithm>
#include <fstream>

namespace ogb {
namespace application {
using namespace ogb::theme;


//! \brief Height of the status bar, in unscaled pixels.
static constexpr float STATUS_BAR_HEIGHT = 26.0f;

// ----------------------------------------------------------------------------
DearImGuiApplication::~DearImGuiApplication()
{
    teardown();
}

// ----------------------------------------------------------------------------
bool DearImGuiApplication::setup(std::string const& iniFilename)
{
    if (m_initialized)
        return true;

    m_window = glfwGetCurrentContext();
    if (m_window == nullptr)
        return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    m_ini_filename = iniFilename;
    io.IniFilename = m_ini_filename.empty() ? nullptr : m_ini_filename.c_str();

    // No settings file yet means a first run, and the panels would otherwise
    // pile up in the middle of the screen. Detect it here rather than by
    // probing the dock node, which already exists by the time the layout could
    // be rebuilt.
    m_build_default_layout =
        m_ini_filename.empty() || !std::ifstream(m_ini_filename).good();

    theme::apply();

    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
    {
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330"))
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_initialized = true;
    return true;
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::teardown()
{
    if (!m_initialized)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    m_window = nullptr;
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::setClearColor(float r, float g, float b)
{
    m_clear_color[0] = r;
    m_clear_color[1] = g;
    m_clear_color[2] = b;
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::draw()
{
    if (!m_initialized)
        return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    drawDockspace();

    if (m_panels_callback)
    {
        m_panels_callback();
    }

    drawStatusBar();
    serveFileDialogs();

    ImGui::Render();

    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::requestFileDialog(FileDialogRequest request)
{
    auto sameKey = [&request](FileDialogRequest const& it) {
        return it.key == request.key;
    };

    // Asking again for a dialog already on screen just brings it forward.
    if (std::any_of(m_open_dialogs.begin(), m_open_dialogs.end(), sameKey) ||
        std::any_of(m_pending_dialogs.begin(), m_pending_dialogs.end(), sameKey))
    {
        return;
    }

    m_pending_dialogs.push_back(std::move(request));
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::serveFileDialogs()
{
    for (auto& request: m_pending_dialogs)
    {
        IGFD::FileDialogConfig config;
        config.path = request.startPath;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog(
            request.key, request.title, request.filters.c_str(), config);

        m_open_dialogs.push_back(std::move(request));
    }
    m_pending_dialogs.clear();

    // Display() has to be reached every frame for as long as the dialog lives,
    // and it returns true only on the frame the user closes it.
    for (size_t i = m_open_dialogs.size(); i-- != 0u;)
    {
        FileDialogRequest& request = m_open_dialogs[i];

        if (!ImGuiFileDialog::Instance()->Display(request.key,
                                                  ImGuiWindowFlags_NoCollapse,
                                                  ImVec2(620.0f, 420.0f)))
        {
            continue;
        }

        if (ImGuiFileDialog::Instance()->IsOk() && request.onAccepted)
        {
            // Copy the callback out: serving it may load a script, which can
            // request another dialog and reallocate the vector under us.
            auto const callback = request.onAccepted;
            std::string const path =
                ImGuiFileDialog::Instance()->GetFilePathName();

            ImGuiFileDialog::Instance()->Close();
            m_open_dialogs.erase(m_open_dialogs.begin() + long(i));

            callback(path);
            continue;
        }

        ImGuiFileDialog::Instance()->Close();
        m_open_dialogs.erase(m_open_dialogs.begin() + long(i));
    }
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::drawDockspace()
{
    ImGuiViewport const* const viewport = ImGui::GetMainViewport();

    // Leave room at the bottom for the status bar.
    ImVec2 const size(viewport->WorkSize.x,
                      viewport->WorkSize.y - STATUS_BAR_HEIGHT);

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags const window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar())
    {
        if (m_menu_bar_callback)
        {
            m_menu_bar_callback();
        }
        ImGui::EndMenuBar();
    }

    ImGuiID const dockspace = ImGui::GetID("OpenGlassBoxDockSpace");

    if (m_build_default_layout)
    {
        m_build_default_layout = false;

        if (m_default_layout_callback)
        {
            ImGui::DockBuilderRemoveNode(dockspace);
            ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace, size);

            m_default_layout_callback(dockspace);

            ImGui::DockBuilderFinish(dockspace);
        }
    }

    ImGui::DockSpace(dockspace, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();
}

// ----------------------------------------------------------------------------
void DearImGuiApplication::drawStatusBar() const
{
    if (!m_status_bar_callback)
        return;

    ImGuiViewport const* const viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(
        viewport->WorkPos.x,
        viewport->WorkPos.y + viewport->WorkSize.y - STATUS_BAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, STATUS_BAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags const flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    if (ImGui::Begin("##StatusBar", nullptr, flags))
    {
        m_status_bar_callback();
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
}
} // namespace application
} // namespace ogb
