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

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return ;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);

    m_success = true;
}

Window::~Window()
{
    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

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
        // Poll and handle events (inputs, window resize, etc.)
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
                    quit = true;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    quit = true;
                } else {
                    game.onKeyDown(event.key.keysym.scancode);
                }
                break;
            default:
                break;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Clear screen
        SDL_SetRenderDrawColor(m_renderer, r, g, b, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);

        // Game rendering
        game.onPaint(*m_renderer, 0.01f);// FIXME 0.01f

        // Rendering ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);

        SDL_RenderPresent(m_renderer);

        // FIXME
        SDL_Delay(100); // ms
    }

    game.onRelease(*m_renderer);
    return true;
}
