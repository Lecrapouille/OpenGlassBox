//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Theme.hpp"
#include "Core/DataPath.hpp"

#include <algorithm>

namespace ogb {
namespace theme {

// ----------------------------------------------------------------------------
void apply()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.SeparatorTextBorderSize = 2.0f;

    ImVec4* const colors = style.Colors;

    ImVec4 const background(0.09f, 0.10f, 0.12f, 1.00f);
    ImVec4 const surface(0.13f, 0.14f, 0.17f, 1.00f);
    ImVec4 const surfaceHigh(0.17f, 0.19f, 0.23f, 1.00f);
    ImVec4 const accent(0.34f, 0.61f, 0.84f, 1.00f);
    ImVec4 const accentDim(0.34f, 0.61f, 0.84f, 0.45f);
    ImVec4 const text(0.90f, 0.92f, 0.95f, 1.00f);

    colors[ImGuiCol_WindowBg] = background;
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = surface;
    colors[ImGuiCol_MenuBarBg] = surface;
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.08f);

    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.53f, 0.58f, 1.00f);

    colors[ImGuiCol_FrameBg] = surface;
    colors[ImGuiCol_FrameBgHovered] = surfaceHigh;
    colors[ImGuiCol_FrameBgActive] = accentDim;

    colors[ImGuiCol_TitleBg] = surface;
    colors[ImGuiCol_TitleBgActive] = surfaceHigh;
    colors[ImGuiCol_TitleBgCollapsed] = surface;

    colors[ImGuiCol_Header] = surfaceHigh;
    colors[ImGuiCol_HeaderHovered] = accentDim;
    colors[ImGuiCol_HeaderActive] = accent;

    colors[ImGuiCol_Button] = surfaceHigh;
    colors[ImGuiCol_ButtonHovered] = accentDim;
    colors[ImGuiCol_ButtonActive] = accent;

    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.71f, 0.94f, 1.00f);
    colors[ImGuiCol_CheckMark] = accent;

    colors[ImGuiCol_Separator] = ImVec4(1.0f, 1.0f, 1.0f, 0.10f);
    colors[ImGuiCol_SeparatorHovered] = accentDim;
    colors[ImGuiCol_SeparatorActive] = accent;

    colors[ImGuiCol_Tab] = surface;
    colors[ImGuiCol_TabHovered] = accentDim;
    colors[ImGuiCol_TabActive] = surfaceHigh;
    colors[ImGuiCol_TabUnfocused] = surface;
    colors[ImGuiCol_TabUnfocusedActive] = surfaceHigh;

    colors[ImGuiCol_PlotLines] = accent;
    colors[ImGuiCol_PlotHistogram] = accent;

    colors[ImGuiCol_DockingPreview] = accentDim;
    colors[ImGuiCol_DockingEmptyBg] = background;

    colors[ImGuiCol_TableHeaderBg] = surfaceHigh;
    colors[ImGuiCol_TableBorderStrong] = ImVec4(1.0f, 1.0f, 1.0f, 0.12f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
}

// ----------------------------------------------------------------------------
void loadFonts(DataPath const& path)
{
    ImGuiIO& io = ImGui::GetIO();

    // Any of these is fine: the demo only needs a readable sans-serif. The
    // built-in ImGui font is used when none is available.
    char const* const candidates[] = {
        "Fonts/Roboto-Regular.ttf",
        "Fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
    };

    for (char const* const candidate: candidates)
    {
        auto const found = path.find(candidate);
        if (!found.second)
            continue;

        if (io.Fonts->AddFontFromFileTTF(found.first.c_str(), 16.0f) != nullptr)
            return;
    }

    io.Fonts->AddFontDefault();
}

// ----------------------------------------------------------------------------
ImU32 fromScript(uint32_t color, float alpha)
{
    alpha = std::min(1.0f, std::max(0.0f, alpha));

    return IM_COL32((color >> 16) & 0xFF,
                    (color >> 8) & 0xFF,
                    (color >> 0) & 0xFF,
                    int(alpha * 255.0f));
}

// ----------------------------------------------------------------------------
ImU32 congestionColor(float ratio, float alpha)
{
    ratio = std::min(1.0f, std::max(0.0f, ratio));
    alpha = std::min(1.0f, std::max(0.0f, alpha));

    // Green to amber to red, so that a way close to its capacity stands out
    // before it is actually saturated.
    float r;
    float g;
    if (ratio < 0.5f)
    {
        float const t = ratio * 2.0f;
        r = 0.35f + 0.65f * t;
        g = 0.75f;
    }
    else
    {
        float const t = (ratio - 0.5f) * 2.0f;
        r = 1.0f;
        g = 0.75f - 0.60f * t;
    }

    return IM_COL32(int(r * 255.0f), int(g * 255.0f), 60, int(alpha * 255.0f));
}

} // namespace theme
} // namespace ogb
