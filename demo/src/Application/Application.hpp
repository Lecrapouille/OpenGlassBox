//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_DEMO_APPLICATION_HPP
#  define OPEN_GLASSBOX_DEMO_APPLICATION_HPP

#  include "Application/DearImGuiApplication.hpp"

#  include <memory>
#  include <string>

namespace ogb {
namespace application {


// ****************************************************************************
//! \brief Host of the demo: owns the GLFW window, the OpenGL context and the
//! main loop. Derived classes only implement the hooks and never create a
//! window or an ImGui context themselves.
// ****************************************************************************
class Application
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] width: initial window width, in pixels.
    //! \param[in] height: initial window height, in pixels.
    //! \param[in] title: initial window title.
    // ------------------------------------------------------------------------
    Application(int width, int height, std::string title);

    // ------------------------------------------------------------------------
    //! \brief Destructor releasing the window and GLFW.
    // ------------------------------------------------------------------------
    virtual ~Application();

    // ------------------------------------------------------------------------
    //! \brief Start the application and run the main loop. Blocking until the
    //! application is halted.
    //! \return true when the application closed normally, false when the setup
    //! failed. In the latter case error() tells why.
    // ------------------------------------------------------------------------
    bool run();

    // ------------------------------------------------------------------------
    //! \brief Reason of the failure of run().
    // ------------------------------------------------------------------------
    std::string const& error() const { return m_error; }

    // ------------------------------------------------------------------------
    //! \brief Change the title shown by the window manager. Cheap to call every
    //! frame: the call is ignored when the title did not change.
    // ------------------------------------------------------------------------
    void setTitle(std::string const& title);

    // ------------------------------------------------------------------------
    //! \brief Close the application unconditionally, without asking
    //! onCloseRequested().
    // ------------------------------------------------------------------------
    void halt();

protected:

    // ------------------------------------------------------------------------
    //! \brief Called once, after the window, the OpenGL context and ImGui have
    //! been created.
    //! \return false to abort the startup.
    // ------------------------------------------------------------------------
    virtual bool onSetup() { return true; }

    // ------------------------------------------------------------------------
    //! \brief Called once, before the window is destroyed.
    // ------------------------------------------------------------------------
    virtual void onTeardown() {}

    // ------------------------------------------------------------------------
    //! \brief Called when the window manager asks to close the window. Return
    //! false to keep the application alive, typically while a confirmation
    //! popup is shown.
    // ------------------------------------------------------------------------
    virtual bool onCloseRequested() { return true; }

    // ------------------------------------------------------------------------
    //! \brief Called every frame before drawing.
    //! \param[in] dt: elapsed wall clock time since the previous frame, in
    //! seconds.
    // ------------------------------------------------------------------------
    virtual void onUpdate(float dt) = 0;

    // ------------------------------------------------------------------------
    //! \brief Called from inside the dockspace menu bar.
    // ------------------------------------------------------------------------
    virtual void onDrawMenuBar() {}

    // ------------------------------------------------------------------------
    //! \brief Called every frame to draw the dockable panels.
    // ------------------------------------------------------------------------
    virtual void onDrawPanels() {}

    // ------------------------------------------------------------------------
    //! \brief Called every frame to fill the status bar.
    // ------------------------------------------------------------------------
    virtual void onDrawStatusBar() {}

    // ------------------------------------------------------------------------
    //! \brief Access to the ImGui layer, to change the clear color.
    // ------------------------------------------------------------------------
    DearImGuiApplication& imgui() { return *m_imgui; }

    // ------------------------------------------------------------------------
    //! \brief Where the dock layout is persisted. Override to change it.
    // ------------------------------------------------------------------------
    virtual std::string imguiIniFilename() const { return "imgui.ini"; }

private:

    bool setup();
    void teardown();
    bool shallBeHalted();
    bool initializeGLFW();
    bool createWindow();
    bool initializeGlew();

protected:

    //! \brief Reason of the failure of run().
    std::string m_error;

private:

    int m_width;
    int m_height;
    std::string m_title;
    GLFWwindow* m_window = nullptr;
    //! \brief Set by halt(), closes without asking onCloseRequested().
    bool m_halted = false;
    std::unique_ptr<DearImGuiApplication> m_imgui;
};
} // namespace application
} // namespace ogb

#endif
