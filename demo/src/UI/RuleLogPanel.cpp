//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "UI/Panels.hpp"
#include "UI/Theme.hpp"
#include "Game/RuleTrace.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>

namespace ogb {
namespace ui {
using namespace ogb::theme;


// ----------------------------------------------------------------------------
static bool containsInsensitive(std::string const& haystack,
                                  std::string const& needle)
{
    if (needle.empty())
        return true;

    auto const it = std::search(
        haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });

    return it != haystack.end();
}

// ----------------------------------------------------------------------------
void RuleLogPanel::draw(Simulation& simulation, game::DebugState& state,
                        game::RuleTrace& trace)
{
    if (!ImGui::Begin("Rule Log"))
    {
        ImGui::End();
        return;
    }

    bool recording = trace.recording();
    if (ImGui::Checkbox("Record", &recording))
    {
        trace.setRecording(recording);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "A rule is attempted on every cell of every layer at every tick,\n"
            "so recording costs something. It is off by default.");
    }

    ImGui::SameLine();
    bool failuresOnly = trace.failuresOnly();
    if (ImGui::Checkbox("Failures only", &failuresOnly))
    {
        trace.setFailuresOnly(failuresOnly);
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        trace.clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Follow", &m_auto_scroll);

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##filter", "filter on the entity or the rule",
                             &m_filter);

    ImGui::Checkbox("ok", &m_show_success);
    ImGui::SameLine();
    ImGui::Checkbox("blocked", &m_show_failure);
    ImGui::SameLine();
    ImGui::TextDisabled("%zu kept, %llu recorded", trace.size(),
                        (unsigned long long)trace.totalRecorded());

    ImGui::Separator();

    if (!recording && (trace.size() == 0u))
    {
        ImGui::TextWrapped(
            "The engine validates every command of a rule and gives up "
            "silently on the first refusal, so a simulation that stays still "
            "gives no clue. Turn on the recording: each line below names the "
            "command that blocked.");
        ImGui::End();
        return;
    }

    ImGuiTableFlags const flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("log", 5, flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("tick", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("city", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("entity");
        ImGui::TableSetupColumn("rule");
        ImGui::TableSetupColumn("outcome");
        ImGui::TableHeadersRow();

        for (size_t i = 0u; i < trace.size(); ++i)
        {
            game::RuleEvent const& event = trace.at(i);

            if (event.success && !m_show_success)
                continue;
            if (!event.success && !m_show_failure)
                continue;
            if (!containsInsensitive(event.entity, m_filter) &&
                !containsInsensitive(event.rule, m_filter))
                continue;

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)event.tick);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(event.city.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s (%u, %u)", event.entity.c_str(), event.u, event.v);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(event.rule.c_str());

            ImGui::TableNextColumn();
            if (event.success)
            {
                ImGui::TextColored(
                    ImGui::ColorConvertU32ToFloat4(theme::SUCCESS), "applied");
            }
            else
            {
                ImGui::TextColored(
                    ImGui::ColorConvertU32ToFloat4(theme::FAILURE),
                    "blocked by %s", event.blockedBy.c_str());
            }
        }

        if (m_auto_scroll && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
