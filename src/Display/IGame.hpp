#ifndef IGAME_HPP
#define IGAME_HPP

#include <SDL2/SDL.h>
#include <cstdint>

class IGame
{
    friend class Window;

public:

    virtual ~IGame() = default;

private:

    virtual bool onInit(SDL_Renderer& renderer) = 0;
    virtual void onRelease(SDL_Renderer& renderer) = 0;
    virtual void onPaint(SDL_Renderer& renderer, float dt) = 0;
    virtual void onKeyDown(int key) = 0;
};

#endif
