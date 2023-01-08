#ifndef IGAME_HPP
#define IGAME_HPP

#include <SDL2/SDL.h>
#include <cstdint>

//==============================================================================
//! \brief Interface for doing a game. Use Window to make run the game.
//==============================================================================
class IGame
{
    friend class Window;

public:

    virtual ~IGame() = default;

private:

    // -------------------------------------------------------------------------
    //! \brief Called by Window to initialize your game.
    // -------------------------------------------------------------------------
    virtual bool onInit(SDL_Renderer& renderer) = 0;

    // -------------------------------------------------------------------------
    //! \brief Called by Window to destroy your game.
    // -------------------------------------------------------------------------
    virtual void onRelease(SDL_Renderer& renderer) = 0;

    // -------------------------------------------------------------------------
    //! \brief Called by Window to render your game.
    // -------------------------------------------------------------------------
    virtual void onPaint(SDL_Renderer& renderer, float dt) = 0;

    // -------------------------------------------------------------------------
    //! \brief Called by Window to initialize your game.
    // -------------------------------------------------------------------------
    virtual void onKeyDown(int key) = 0;
};

#endif
