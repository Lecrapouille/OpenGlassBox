//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "main.hpp"
#include "Display/SDLHelper.hpp"

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
        rect.x = 10 * int(it->position().x);
        rect.y = 10 * int(it->position().y);
        rect.w = 5; // GRID_SIZE
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
        rect.x = 10 * int(it->position().x);
        rect.y = 10 * int(it->position().y);
        rect.w = 10; // GRID_SIZE
        rect.h = 10;
        SDL_RenderFillRect(&renderer, &rect);

        drawText(&renderer, m_fontTexture, rect.x, rect.y,
                 RED(it->color()), GREEN(it->color()), BLUE(it->color()),
                 TEXT_LEFT, "%u", it->id());
    }
}

//------------------------------------------------------------------------------
void GlassBox::drawMaps(City const& /*city*/, SDL_Renderer& /*renderer*/)
{
    // TODO
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
                               10 * int(it->position1().x),
                               10 * int(it->position1().y),
                               10 * int(it->position2().x),
                               10 * int(it->position2().y));
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
            rect.x = 10 * int(it->position().x);
            rect.y = 10 * int(it->position().y);
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
