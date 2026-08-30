//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Theme.hpp
//! \brief Dark ImGui theme, fonts and color helpers for the demo UI.


#ifndef OPEN_GLASSBOX_DEMO_THEME_HPP
#  define OPEN_GLASSBOX_DEMO_THEME_HPP

#  include "Host/OpenGL.hpp"

namespace ogb::theme
{

//! \brief Accent used for the selection and the active widgets.
ImU32 const ACCENT = IM_COL32(86, 156, 214, 255);
//! \brief Feedback colors of the rule log and of the inspector.
ImU32 const SUCCESS = IM_COL32(106, 190, 120, 255);
ImU32 const FAILURE = IM_COL32(224, 108, 117, 255);
ImU32 const MUTED = IM_COL32(150, 155, 165, 255);
//! \brief Background of the city canvas.
ImU32 const CANVAS_BACKGROUND = IM_COL32(24, 26, 31, 255);

// ----------------------------------------------------------------------------
//! \brief Canvas colour that follows the in-game hour: night, dawn, day, dusk.
//! \param[in] hourOfDay: fractional hour in [0, 24), minutes included.
// ----------------------------------------------------------------------------
ImU32 canvasBackground(float hourOfDay);

// ----------------------------------------------------------------------------
//! \brief Colour of the clock HUD, warm by day and cool at night.
// ----------------------------------------------------------------------------
ImU32 clockHudColor(float hourOfDay);
//! \brief Grid lines drawn over the layers.
ImU32 const GRID_LINE = IM_COL32(255, 255, 255, 18);
ImU32 const GRID_LINE_STRONG = IM_COL32(255, 255, 255, 40);

// ----------------------------------------------------------------------------
//! \brief Install the dark style of the demo: rounded frames, roomy spacing and
//! a palette consistent with the simulation colors.
// ----------------------------------------------------------------------------
void apply();

// ----------------------------------------------------------------------------
//! \brief Load a readable sans-serif when a system font is available, otherwise
//! the built-in ImGui font.
// ----------------------------------------------------------------------------
void loadFonts();

// ----------------------------------------------------------------------------
//! \brief Convert a 0xRRGGBB color coming from a simulation script into an
//! ImGui packed color.
//! \param[in] color: the 0xRRGGBB value.
//! \param[in] alpha: opacity in [0..1].
// ----------------------------------------------------------------------------
ImU32 fromScript(uint32_t color, float alpha = 1.0f);

// ----------------------------------------------------------------------------
//! \brief Layer a ratio in [0..1] onto a green to red gradient, used for the
//! saturation of the ways and for the heatmaps.
// ----------------------------------------------------------------------------
ImU32 congestionColor(float ratio, float alpha = 1.0f);

} // namespace ogb::theme

#endif
