//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "main.hpp"
#include "Display/SDLHelper.hpp"
#include "Config.hpp"
#include <algorithm>

//------------------------------------------------------------------------------
#define RED(color)   ((color >> 16) & 0xFF)
#define GREEN(color) ((color >> 8) & 0xFF)
#define BLUE(color)  ((color >> 0) & 0xFF)

//------------------------------------------------------------------------------
void GlassBox::drawAgents(City const& city, SDL_Renderer& renderer)
{
    for (auto& it: city.agents())
    {
        SDL_SetRenderDrawColor(&renderer,
                               RED(it->color()),
                               GREEN(it->color()),
                               BLUE(it->color()),
                               SDL_ALPHA_OPAQUE);
        SDL_Rect rect;
        rect.x = int(/*city.position().x +*/ it->position().x);
        rect.y = int(/*city.position().y +*/ it->position().y);
        rect.w = 5;
        rect.h = 5;
        SDL_RenderFillRect(&renderer, &rect);

        drawText(&renderer, m_fontTexture, rect.x, rect.y,
                 RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                 TEXT_LEFT, "%u", it->id());
    }
}

//------------------------------------------------------------------------------
void GlassBox::drawUnits(City const& city, SDL_Renderer& renderer)
{
    for (auto& it: city.units())
    {
        SDL_SetRenderDrawColor(&renderer,
                               RED(it->color()),
                               GREEN(it->color()),
                               BLUE(it->color()),
                               SDL_ALPHA_OPAQUE);
        SDL_Rect rect;
        rect.x = int(/*city.position().x +*/ it->position().x);
        rect.y = int(/*city.position().y +*/ it->position().y);
        rect.w = 10;
        rect.h = 10;
        SDL_RenderFillRect(&renderer, &rect);

        drawText(&renderer, m_fontTexture, rect.x, rect.y,
                 RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                 TEXT_LEFT, "%u", it->id());
    }
}

struct R
{
    R(uint32_t c, float r)
        : color(c), ratio(r)
    {}
    uint32_t color;
    float ratio;
};

//------------------------------------------------------------------------------
void GlassBox::drawMaps(City const& city, SDL_Renderer& renderer)
{
    for (auto& it: city.maps())
    {
        Map& map = (*it.second);

        // Display a filled rectangle with a ratio amount / capacity of resources
        SDL_SetRenderDrawColor(&renderer,
                               RED(map.color()),
                               GREEN(map.color()),
                               BLUE(map.color()),
                               SDL_ALPHA_OPAQUE);

        uint32_t capacity = map.getCapacity();
        for (uint32_t u = 0; u < map.gridSizeU(); ++u)
        {
            for (uint32_t v = 0; v < map.gridSizeV(); ++v)
            {
                SDL_Rect rect;
                rect.x = int(config::GRID_SIZE * u) + int(map.position().x);
                rect.y = int(config::GRID_SIZE * v) + int(map.position().y);
                rect.w = int(config::GRID_SIZE * map.getResource(u, v) / capacity);
                rect.h = rect.w;
                SDL_RenderFillRect(&renderer, &rect);
            }
        }

        // Display the grid
        drawText(&renderer, m_fontTexture, city.position().x, city.position().y,
             0, 0, 0, TEXT_LEFT, "%s", city.name().c_str());
        SDL_SetRenderDrawColor(&renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

        uint32_t u = map.gridSizeU() + 1u;
        uint32_t v = map.gridSizeV() + 1u;
        int32_t max_x = int32_t(map.position().x) + int32_t(map.gridSizeU() * config::GRID_SIZE);
        int32_t max_y = int32_t(map.position().y) + int32_t(map.gridSizeV() * config::GRID_SIZE);
        int32_t x = max_x;
        int32_t y = max_y;

        while (u--)
        {
            SDL_RenderDrawLine(&renderer, x, map.position().y, x, max_y);
            x -= int32_t(config::GRID_SIZE);
        }

        while (v--)
        {
            SDL_RenderDrawLine(&renderer, map.position().x, y, max_x, y);
            y -= int32_t(config::GRID_SIZE);
        }
    }
}

//------------------------------------------------------------------------------
// TODO use SDL_RenderDrawLines to draw a batch of segments
void GlassBox::drawPaths(City const& city, SDL_Renderer& renderer)
{
    for (auto& path: city.paths())
    {
        // Draw arcs
        for (auto& it: path.second->ways())
        {
            SDL_SetRenderDrawColor(&renderer,
                                   RED(it->color()),
                                   GREEN(it->color()),
                                   BLUE(it->color()),
                                   SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(&renderer,
                               int(/*city.position().x +*/ it->position1().x),
                               int(/*city.position().y +*/ it->position1().y),
                               int(/*city.position().x +*/ it->position2().x),
                               int(/*city.position().y +*/ it->position2().y));
        }

        // Draw nodes
        for (auto& it: path.second->nodes())
        {
            SDL_SetRenderDrawColor(&renderer,
                                   RED(it->color()),
                                   GREEN(it->color()),
                                   BLUE(it->color()),
                                   SDL_ALPHA_OPAQUE);
            SDL_Rect rect;
            rect.x = int(/*city.position().x +*/ it->position().x);
            rect.y = int(/*city.position().y +*/ it->position().y);
            rect.w = 5;
            rect.h = 5;
            SDL_RenderFillRect(&renderer, &rect);

            drawText(&renderer, m_fontTexture, rect.x, rect.y,
                     RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                     TEXT_LEFT, "%u", it->id());
        }
    }
}

//------------------------------------------------------------------------------
void GlassBox::drawCity(City const& city, SDL_Renderer& renderer)
{
    // TODO
    // Draw the grid
    // Draw the 1st map (todo rectangle * capacity/amount)
    // Draw Areas

    drawMaps(city, renderer);
    drawPaths(city, renderer);
    drawUnits(city, renderer);
    drawAgents(city, renderer);
}

//------------------------------------------------------------------------------
void GlassBox::renderSimulation(SDL_Renderer& renderer)
{
    for (auto const& city: m_simulation.cities())
        drawCity(*(city.second), renderer);
}
