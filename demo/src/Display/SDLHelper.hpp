//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef SDLHELPER_HPP
#  define SDLHELPER_HPP

#  include <SDL2/SDL.h>
#  include <SDL_image.h>

enum class TextAlignment
{
    TEXT_LEFT,
    TEXT_CENTER,
    TEXT_RIGHT
};

//------------------------------------------------------------------------------
//! \brief Draw a text from a texture.
//! \param[in] renderer: SDL renderer,
//! \param[in] fontTexture: SDL struct holding the texture of the fonts.
//! \param[in] x: position of the 1st char.
//! \param[in] y: position of the 1st char.
//! \param[in] r: red color.
//! \param[in] g: green color.
//! \param[in] b: blue color.
//! \param[in] align: horizontal alignment.
//! \param[in] format: printf style to draw text
//------------------------------------------------------------------------------
void drawText(SDL_Renderer* renderer, SDL_Texture* fontTexture,
              int x, int y, Uint8 r, Uint8 g, Uint8 b,
              TextAlignment align, const char *format, ...);


#endif
