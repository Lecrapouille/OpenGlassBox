//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//
// Adapted from the Oakular application layer of
// https://github.com/Lecrapouille/BlackThorn
//-----------------------------------------------------------------------------

//! \file DearImGuiApplication.hpp
//! \brief ImGui frame setup: docking layout, menus, dialogs and status bar.


#ifndef OPEN_GLASSBOX_DEMO_DEAR_IMGUI_APPLICATION_HPP
#  define OPEN_GLASSBOX_DEMO_DEAR_IMGUI_APPLICATION_HPP

#  include "Host/OpenGL.hpp"

#  include <functional>
#  include <string>
#  include <vector>

namespace ogb::host
{


// ****************************************************************************
//! \brief A file dialog a panel asks the host to open.
//!
//! Panels do not open the dialog themselves: a modal has to be displayed by
//! whoever owns the frame, and only the host knows the frame is still being
//! built when a menu item is clicked. Emitting a request instead keeps the file
//! picker out of every panel that needs a file.
// ****************************************************************************
struct FileDialogRequest
{
    //! \brief Identifies the dialog across frames. Two requests sharing a key
    //! are the same dialog, so asking twice does not stack two modals.
    std::string key;
    std::string title;
    //! \brief ImGuiFileDialog filter string, for instance ".txt,.*".
    std::string filters = ".*";
    std::string startPath = ".";
    //! \brief Called with the chosen path when the user validates. Not called
    //! when the dialog is cancelled.
    std::function<void(std::string const&)> onAccepted;
};

// ****************************************************************************
//! \brief Dear ImGui application wrapper providing docking support. Owns the
//! ImGui context and the GLFW/OpenGL3 backends, and lays out a full viewport
//! dockspace with a menu bar.
// ****************************************************************************
class DearImGuiApplication
{
public:

    using Callback = std::function<void()>;
    //! \brief Receives the identifier of the dockspace node to fill.
    using LayoutCallback = std::function<void(ImGuiID)>;

    // ------------------------------------------------------------------------
    //! \brief Destructor releasing the ImGui context if still alive.
    // ------------------------------------------------------------------------
    ~DearImGuiApplication();

    // ------------------------------------------------------------------------
    //! \brief Called from inside the dockspace menu bar.
    // ------------------------------------------------------------------------
    void setMenuBarCallback(Callback&& callback)
    {
        m_menu_bar_callback = std::move(callback);
    }

    // ------------------------------------------------------------------------
    //! \brief Called once per frame to draw every dockable panel.
    // ------------------------------------------------------------------------
    void setPanelsCallback(Callback&& callback)
    {
        m_panels_callback = std::move(callback);
    }

    // ------------------------------------------------------------------------
    //! \brief Called once per frame, after the panels, to draw the status bar
    //! docked at the bottom of the viewport.
    // ------------------------------------------------------------------------
    void setStatusBarCallback(Callback&& callback)
    {
        m_status_bar_callback = std::move(callback);
    }

    // ------------------------------------------------------------------------
    //! \brief Called once, on the first run only, to arrange the panels inside
    //! the dockspace. Later runs restore the layout the user left behind.
    // ------------------------------------------------------------------------
    void setDefaultLayoutCallback(LayoutCallback&& callback)
    {
        m_default_layout_callback = std::move(callback);
    }

    // ------------------------------------------------------------------------
    //! \brief Arrange the panels again as on a first run, on the next frame.
    //! A layout saved by an older version knows nothing of a panel added
    //! since, which would otherwise float undocked for good.
    // ------------------------------------------------------------------------
    void resetLayout()
    {
        m_build_default_layout = true;
    }

    // ------------------------------------------------------------------------
    //! \brief Initialize the ImGui context and its backends.
    //! \param[in] iniFilename: where the dock layout is persisted. Pass an
    //! empty string to disable the persistence.
    //! \return true if the initialization succeeded.
    // ------------------------------------------------------------------------
    bool setup(std::string const& iniFilename);

    // ------------------------------------------------------------------------
    //! \brief Release the ImGui context and its backends.
    // ------------------------------------------------------------------------
    void teardown();

    // ------------------------------------------------------------------------
    //! \brief Draw a whole frame: dockspace, panels, status bar, then submit
    //! the draw data to OpenGL.
    // ------------------------------------------------------------------------
    void draw();

    // ------------------------------------------------------------------------
    //! \brief Color the framebuffer is cleared with, as RGB in [0..1].
    // ------------------------------------------------------------------------
    void setClearColor(float r, float g, float b);

    // ------------------------------------------------------------------------
    //! \brief Ask the host to open a file dialog. Safe to call from inside a
    //! menu or a panel: the dialog is opened at the end of the frame.
    // ------------------------------------------------------------------------
    void requestFileDialog(FileDialogRequest request);

private:

    // ------------------------------------------------------------------------
    //! \brief Open the dialogs requested during this frame and display the ones
    //! still open, calling back the requester when a file is validated.
    // ------------------------------------------------------------------------
    void serveFileDialogs();

private:

    // ------------------------------------------------------------------------
    //! \brief Lay out the full viewport dockspace holding the menu bar.
    // ------------------------------------------------------------------------
    void drawDockspace();

    // ------------------------------------------------------------------------
    //! \brief Draw the status bar as a viewport docked bar.
    // ------------------------------------------------------------------------
    void drawStatusBar() const;

private:

    //! \brief Window owning the OpenGL context, taken from the current context.
    GLFWwindow* m_window = nullptr;
    //! \brief Persisted so that the string outlives ImGuiIO::IniFilename.
    std::string m_ini_filename;
    bool m_initialized = false;
    float m_clear_color[3] = { 0.09f, 0.10f, 0.12f };

    //! \brief True until the default layout has been built. Stays false for
    //! good when an ini file was found, so that the arrangement of the user is
    //! never overwritten.
    bool m_build_default_layout = false;

    Callback m_menu_bar_callback;
    Callback m_panels_callback;
    Callback m_status_bar_callback;
    LayoutCallback m_default_layout_callback;

    //! \brief Dialogs asked for during this frame, not yet opened.
    std::vector<FileDialogRequest> m_pending_dialogs;
    //! \brief Dialogs currently on screen, kept until they are closed because
    //! the callback has to outlive the frame that asked for them.
    std::vector<FileDialogRequest> m_open_dialogs;
};
} // namespace ogb::host

#endif
