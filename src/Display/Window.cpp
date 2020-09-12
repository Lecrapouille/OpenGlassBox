#include "Window.hpp"
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

Window::Window()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        m_success = false;
    }
    else
    {
        if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
                                        &m_window, &m_renderer) < 0)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            m_success = false;
        }
        else
        {
            m_success = true;
        }
    }
}

Window::~Window()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Window::run(IGame& game)
{
    bool quit = false;
    SDL_Event event;

    if (!m_success)
        return false;

    if (!game.onInit())
        return false;

    while (!quit)
    {
        // Background color
        SDL_SetRenderDrawColor(m_renderer, r, g, b, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);

        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = 1;
                break;

            case SDL_KEYDOWN:
                game.onKeyDown(event.key.keysym.scancode);
                break;
            }
        }

        game.onPaint(*m_renderer, 0.01f);
        SDL_RenderPresent(&m_renderer);
        SDL_Delay(100); // ms
    }

    game.onRelease();
    return true;
}
