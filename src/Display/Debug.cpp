#include "Display/Debug.hpp"
#include "Display/DearImGui.hpp"
#include "Core/Simulation.hpp"

static char buffer[1024];

static void debugAgents(City const& city)
{
    if (!ImGui::CollapsingHeader("Agents"))
        return ;

    for (auto const& agent: city.agents())
    {
        sprintf(buffer, "Agent %u (%s):", agent->id(), agent->type().c_str());
        if (!ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
            return ;

        ImGui::TextWrapped("Position %i, %i", int(agent->position().x),
                           int(agent->position().y));
        ImGui::TextWrapped("Has %zu Resources:", agent->resources().size());
        for (auto const& r: agent->resources())
        {
            ImGui::BulletText("%s: %u / %u", r.type().c_str(),
                              r.getAmount(), r.getCapacity());
        }
        ImGui::TreePop();
    }
}

static void debugUnits(City const& city)
{
    if (!ImGui::CollapsingHeader("Units"))
        return ;

    for (auto const& unit: city.units())
    {
        sprintf(buffer, "Unit %u (%s):", unit->id(), unit->type().c_str());
        if (!ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
            return ;

        ImGui::TextWrapped("Node %u. Position %i, %i",
                           unit->node().id(),
                           int(unit->position().x),
                           int(unit->position().y));
        ImGui::TextWrapped("Has %zu Resources:",
                           unit->resources().container().size());
        for (auto const& r: unit->resources().container())
        {
            ImGui::BulletText("%s: %u / %u", r.type().c_str(),
                              r.getAmount(), r.getCapacity());
        }

        ImGui::TreePop();
    }
}

static void debugMaps(City const& city)
{
    if (ImGui::CollapsingHeader("Maps"))
    {
        for (auto const& map: city.maps())
        {
        }
    }
}

static void debugPaths(City const& city)
{

}

void debugCity(City const& city)
{
    if (ImGui::CollapsingHeader(city.name().c_str()))
    {
        debugAgents(city);
        debugUnits(city);
        debugMaps(city);
        debugPaths(city);
    }
}

void debugCities(Simulation const& simulation)
{
    for (auto const& city: simulation.cities())
    {
        debugCity(*(city.second));
    }
}
