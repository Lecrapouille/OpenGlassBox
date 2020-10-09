//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "main.hpp"
#include <SDL_image.h>

#define GLYPH_WIDTH  (18)
#define GLYPH_HEIGHT (29)
#define RED(color)   ((color >> 16) & 0xFF)
#define GREEN(color) ((color >> 8) & 0xFF)
#define BLUE(color)  ((color >> 0) & 0xFF)

GlassBox::GlassBox()
    : m_simulation(32u, 32u)
{}

void GlassBox::onRelease(SDL_Renderer&)
{
    SDL_DestroyTexture(m_fontTexture);
}

void GlassBox::blitRect(SDL_Texture *texture, SDL_Rect *src, int x, int y)
{
    SDL_Rect dest;

    dest.x = x;
    dest.y = y;
    dest.w = src->w;
    dest.h = src->h;

    SDL_RenderCopy(m_renderer, texture, src, &dest);
}

void GlassBox::drawText(int x, int y, Uint8 r, Uint8 g, Uint8 b,
                        TextAlignement align, const char *format, ...)
{
    SDL_Rect rect;
    va_list args;

    memset(&m_drawTextBuffer, '\0', sizeof(m_drawTextBuffer));

    va_start(args, format);
    vsprintf(m_drawTextBuffer, format, args);
    va_end(args);

    int len = int(strlen(m_drawTextBuffer));

    switch (align)
    {
    case TEXT_RIGHT:
        x -= (len * GLYPH_WIDTH);
        break;
    case TEXT_CENTER:
        x -= (len * GLYPH_WIDTH) / 2;
        break;
    case TEXT_LEFT:
    default:
        break;
    }

    rect.w = GLYPH_WIDTH;
    rect.h = GLYPH_HEIGHT;
    rect.y = 0;

    SDL_SetTextureColorMod(m_fontTexture, r, g, b);

    for (int i = 0 ; i < len ; i++)
    {
        int c = m_drawTextBuffer[i];
        if (c >= ' ' && c <= 'Z')
        {
            rect.x = (c - ' ') * GLYPH_WIDTH;
            blitRect(m_fontTexture, &rect, x, y);
            x += GLYPH_WIDTH;
        }
    }
}

bool GlassBox::initSDL(SDL_Renderer& renderer)
{
    m_renderer = &renderer;
    const char *file = "data/Fonts/font.png";
    m_fontTexture = IMG_LoadTexture(m_renderer, file);
    if (m_fontTexture == nullptr) {
        std::cerr << "Failed loading texture " << file << std::endl;
        return false;
    }

    return true;
}

bool GlassBox::initSimulation()
{
    Script script; // TODO: a cacher dans Simulation
    if (!script.parse("data/Simulations/TestCity.txt")) {
        std::cerr << "Failed loading simulation scripts" << std::endl;
        return false;
    }

    City& city = m_simulation.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    Path& road = city.addPath(script.getPathType("Road"));
    Node& n1 = road.addNode(Vector3f(20.0f, 20.0f, 0.0f));
    Node& n2 = road.addNode(Vector3f(50.0f, 50.0f, 0.0f));
    Node& n3 = road.addNode(Vector3f(20.0f, 50.0f, 0.0f));
    Way& w1 = road.addWay(script.getWayType("Dirt"), n1, n2);
    Way& w2 = road.addWay(script.getWayType("Dirt"), n2, n3);
    Way& w3 = road.addWay(script.getWayType("Dirt"), n3, n1);
    Unit& u1 = city.addUnit(script.getUnitType("Home"), road, w1, 0.66f);
    Unit& u2 = city.addUnit(script.getUnitType("Home"), road, w1, 0.5f);
    Unit& u3 = city.addUnit(script.getUnitType("Work"), road, w2, 0.5f);
    Unit& u4 = city.addUnit(script.getUnitType("Work"), road, w3, 0.5f);
    Map& m1 = city.addMap(script.getMapType("Grass"));
    Map& m2 = city.addMap(script.getMapType("Water"));

    return true;
}

bool GlassBox::onInit(SDL_Renderer& renderer)
{
    if (!initSimulation())
        return false;
    return initSDL(renderer);
}

// TODO use SDL_RenderDrawLines to draw a batch of segments
void GlassBox::onPaint(SDL_Renderer& renderer, float dt)
{
    // Update states of the simulation
    m_simulation.update(dt);

    City& city = m_simulation.getCity("Paris");

    // Draw the grid

    // Draw the 1st map (todo rectangle * capacity/amount)

    // Draw Areas

    for (auto& path: city.paths())
    {
        // Draw roads
        for (auto& it: path.second->ways()) // TODO rename to getWays ?
        {
            SDL_SetRenderDrawColor(&renderer,
                                   RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                                   SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(&renderer,
                               10 * int(it->position1().x),
                               10 * int(it->position1().y),
                               10 * int(it->position2().x),
                               10 * int(it->position2().y));
        }

        // Draw nodes
        for (auto& it: path.second->nodes()) // TODO rename to getNodes ?
        {
            SDL_SetRenderDrawColor(&renderer,
                                   RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                                   SDL_ALPHA_OPAQUE);
            SDL_Rect rect;
            rect.x = 10 * int(it->position().x);
            rect.y = 10 * int(it->position().y);
            rect.w = 5;
            rect.h = 5;
            SDL_RenderFillRect(&renderer, &rect);
        }
    }

    // Draw Units
    for (auto& it: city.units())
    {
        SDL_SetRenderDrawColor(&renderer,
                               RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                               SDL_ALPHA_OPAQUE);
        SDL_Rect rect;
        rect.x = 10 * int(it->position().x);
        rect.y = 10 * int(it->position().y);
        rect.w = 10; // GRID_SIZE
        rect.h = 10;
        SDL_RenderFillRect(&renderer, &rect);
    }

    // Draw agents
    for (auto& it: city.agents())
    {
        SDL_SetRenderDrawColor(&renderer,
                               RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                               SDL_ALPHA_OPAQUE);
        SDL_Rect rect;
        rect.x = 10 * int(it->position().x);
        rect.y = 10 * int(it->position().y);
        rect.w = 5; // GRID_SIZE
        rect.h = 5;
        SDL_RenderFillRect(&renderer, &rect);

        drawText(rect.x, rect.y,
                 RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                 TEXT_LEFT,
                 "%u", it->id());
    }
}

void GlassBox::onKeyDown(int key)
{
    switch (key)
    {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
        printf("Touche appuyee\n");
        break;
    }
}

int main()
{
    Window w;
    GlassBox game;

    w.color(100, 100, 100);
    return w.run(game) ? EXIT_SUCCESS : EXIT_FAILURE;
}
