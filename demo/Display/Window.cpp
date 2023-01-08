#include "Display/Window.hpp"
#include "Display/DearImGui.hpp"
#include "OpenGlassBox/Config.hpp"
#include <stdio.h>

Window::Window()
{
    // By default an error :)
    m_success = false;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return ;
    }

    m_window = SDL_CreateWindow("OpenGlassBox",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                config::SCREEN_WIDTH, config::SCREEN_HEIGHT,
                                SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return ;
    }
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (m_renderer == nullptr)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return ;
    }

    ImGui::CreateContext();
    ImGuiSDL::Initialize(m_renderer, config::SCREEN_WIDTH, config::SCREEN_HEIGHT);

    m_success = true;
}

Window::~Window()
{
    ImGuiSDL::Deinitialize();
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

    if (!game.onInit(*m_renderer))
        return false;

    while (!quit)
    {
        // Background color
        SDL_SetRenderDrawColor(m_renderer, r, g, b, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);

        // ImGuiIO I/O events
        ImGuiIO& io = ImGui::GetIO();
        int wheel = 0;

        // SDL I/O events
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_WINDOWEVENT:
                // SDL --> ImGuiIO
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    io.DisplaySize.x = static_cast<float>(event.window.data1);
                    io.DisplaySize.y = static_cast<float>(event.window.data2);
                }
                break;

            case SDL_QUIT:
                quit = 1;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    quit = 1;
                } else {
                    game.onKeyDown(event.key.keysym.scancode);
                }
                break;

            case SDL_MOUSEWHEEL:
                wheel = event.wheel.y;
                break;

            default: break;
            }
        }

        // SDL I/O events --> ImGuiIO I/O events
        int mouseX, mouseY;
        const uint32_t buttons = SDL_GetMouseState(&mouseX, &mouseY);
        io.DeltaTime = 1.0f / 60.0f;
        io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
        io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
        io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
        io.MouseWheel = static_cast<float>(wheel);

        // Painting
        ImGui::NewFrame();
        game.onPaint(*m_renderer, 0.01f);// FIXME 0.01f
        ImGui::Render();
        ImGuiSDL::Render(ImGui::GetDrawData());
        SDL_RenderPresent(m_renderer);

        // FIXME
        SDL_Delay(100); // ms
    }

    game.onRelease(*m_renderer);
    return true;
}
