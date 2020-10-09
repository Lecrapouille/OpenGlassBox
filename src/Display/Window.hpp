#ifndef WINDOW_HPP
#define WINDOW_HPP

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

class Window
{
public:

    Window();
    ~Window();
    bool run(IGame& game);
    void color(uint8_t r, uint8_t g, uint8_t b)
    {
        this->r = r; this->g = g; this->b = b;
    }

private:

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Surface* m_screenSurface = nullptr;
    bool m_success = false;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 100;
};

#endif
