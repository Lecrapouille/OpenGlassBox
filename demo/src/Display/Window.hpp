#ifndef WINDOW_HPP
#  define WINDOW_HPP

#  include "Display/IGame.hpp"

//==============================================================================
//! \brief Create and manage a window made in SDL. This class is made to run a
//!  IGame which is the interface for your game.
//==============================================================================
class Window
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create and initialize a SDL window running DearImGui.
    // -------------------------------------------------------------------------
    Window();

    // -------------------------------------------------------------------------
    //! \brief Release SDL
    // -------------------------------------------------------------------------
    ~Window();

    // -------------------------------------------------------------------------
    //! \brief Make run a game, it calls the IGame::onInit(), manage the game
    //! loop, I/O, call IGame::onPaint() and finally the IGame::onRelease().
    //! \param[inout] game: the game to run.
    //! \return true in case of success, false in case of failure.
    // -------------------------------------------------------------------------
    bool run(IGame& game);

    // -------------------------------------------------------------------------
    //! \brief Change the background color of the Windows.
    // -------------------------------------------------------------------------
    void setBackgroundColor(uint8_t red, uint8_t green, uint8_t blue)
    {
        this->r = red; this->g = green; this->b = blue;
    }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the SDL renderer.
    // -------------------------------------------------------------------------
    SDL_Renderer* renderer() { return m_renderer; }

    // -------------------------------------------------------------------------
    //! \brief Getter: return the windows.
    // -------------------------------------------------------------------------
    SDL_Window* operator()() { return m_window; }

private:

    //! \brief
    SDL_Window* m_window = nullptr;
    //! \brief SDL renderer
    SDL_Renderer* m_renderer = nullptr;
    //! \brief
    bool m_success = false;
    //! \brief background color (red)
    uint8_t r = 0;
    //! \brief background color (green)
    uint8_t g = 0;
    //! \brief background color (blue)
    uint8_t b = 100;
};

#endif
