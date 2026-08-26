//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Host/Application.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace ogb {
namespace host {


//! \brief Upper bound on the frame duration handed to onUpdate(). Without it,
//! a breakpoint or a window drag would hand over a huge delta and make the
//! simulation jump.
static constexpr float MAX_FRAME_TIME = 0.25f;

// ----------------------------------------------------------------------------
Application::Application(int width, int height, std::string title)
    : m_width(width), m_height(height), m_title(std::move(title))
{}

// ----------------------------------------------------------------------------
Application::~Application()
{
    if (m_window != nullptr)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

// ----------------------------------------------------------------------------
void Application::setTitle(std::string const& title)
{
    if (title == m_title)
        return;

    m_title = title;
    if (m_window != nullptr)
    {
        glfwSetWindowTitle(m_window, m_title.c_str());
    }
}

// ----------------------------------------------------------------------------
void Application::halt()
{
    m_halted = true;
    if (m_window != nullptr)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

// ----------------------------------------------------------------------------
bool Application::shallBeHalted()
{
    if (m_halted)
        return true;

    if (m_window == nullptr)
        return true;

    if (glfwWindowShouldClose(m_window) == GLFW_FALSE)
        return false;

    // The window manager asked to close. Take the request back so that the
    // application may refuse it, typically to confirm the loss of unsaved
    // changes first.
    glfwSetWindowShouldClose(m_window, GLFW_FALSE);
    return onCloseRequested();
}

// ----------------------------------------------------------------------------
bool Application::initializeGLFW()
{
    glfwSetErrorCallback([](int code, char const* description) {
        std::cerr << "GLFW error " << code << ": " << description << std::endl;
    });

    // GLFW 3.4+ picks Wayland whenever it is available, and a Wayland window
    // cannot be driven by xdotool or captured by window id. OGB_PLATFORM=x11
    // forces XWayland, which is what makes scripted runs and screenshots
    // possible. Distro packages often ship GLFW 3.3, so guard these hints.
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
    char const* platform = std::getenv("OGB_PLATFORM");
    if (platform != nullptr)
    {
        if (std::string(platform) == "x11")
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        else if (std::string(platform) == "wayland")
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    }
#endif

    if (glfwInit() == GLFW_FALSE)
    {
        m_error = "Failed to initialize GLFW";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    return true;
}

// ----------------------------------------------------------------------------
bool Application::createWindow()
{
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(),
                                nullptr, nullptr);
    if (m_window == nullptr)
    {
        m_error = "Failed to create the GLFW window";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSwapInterval(1); // VSync

    return true;
}

// ----------------------------------------------------------------------------
bool Application::initializeGlew()
{
    glewExperimental = GL_TRUE;
    GLenum const err = glewInit();

    // GLEW leaves a spurious GL_INVALID_ENUM behind on core profiles.
    while (glGetError() != GL_NO_ERROR)
    {
    }

    if ((err != GLEW_OK) && (err != GLEW_ERROR_NO_GLX_DISPLAY))
    {
        if (glGetString(GL_VERSION) == nullptr)
        {
            m_error = "Failed to initialize GLEW: ";
            m_error += reinterpret_cast<char const*>(glewGetErrorString(err));
            glfwTerminate();
            return false;
        }
    }

    if (!GLEW_VERSION_3_3)
    {
        char const* const version =
            reinterpret_cast<char const*>(glGetString(GL_VERSION));
        m_error = "OpenGL 3.3 is not supported. Available: ";
        m_error += (version != nullptr) ? version : "unknown";
        glfwTerminate();
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
bool Application::setup()
{
    if (!initializeGLFW())
        return false;

    if (!createWindow())
        return false;

    if (!initializeGlew())
        return false;

    m_imgui = std::make_unique<DearImGuiApplication>();
    if (!m_imgui->setup(imguiIniFilename()))
    {
        m_error = "Failed to initialize Dear ImGui";
        return false;
    }

    m_imgui->setMenuBarCallback([this]() { this->onDrawMenuBar(); });
    m_imgui->setPanelsCallback([this]() { this->onDrawPanels(); });
    m_imgui->setStatusBarCallback([this]() { this->onDrawStatusBar(); });

    return onSetup();
}

// ----------------------------------------------------------------------------
void Application::teardown()
{
    onTeardown();

    if (m_imgui)
    {
        m_imgui->teardown();
        m_imgui.reset();
    }
}

// ----------------------------------------------------------------------------
bool Application::run()
{
    if (!setup())
    {
        teardown();
        return false;
    }

    auto previous = std::chrono::steady_clock::now();

    while (!shallBeHalted())
    {
        glfwPollEvents();

        auto const now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - previous).count();
        previous = now;

        if (dt > MAX_FRAME_TIME)
        {
            dt = MAX_FRAME_TIME;
        }

        onUpdate(dt);
        m_imgui->draw();
        glfwSwapBuffers(m_window);
    }

    teardown();
    return true;
}
} // namespace host
} // namespace ogb
