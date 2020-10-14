#include "Display/Debug.hpp"
#include "Display/DearImGui.hpp"
#include "Core/Simulation.hpp"
#include <sstream>

#define COLOR(a) ImVec4(                                \
        float(((a)->color() >> 16) & 0xFF),             \
        float(((a)->color() >> 8) & 0xFF),              \
        float(((a)->color() >> 0) & 0xFF),              \
        1.0f)

static char buffer[1024];

static void debugAgents(City const& city)
{
    if (!ImGui::CollapsingHeader("Agents"))
        return ;

    for (auto const& agent: city.agents())
    {
        sprintf(buffer, "Agent %u (%s):", agent->id(), agent->type().c_str());
        if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextColored(COLOR(agent), "%s", agent->type().c_str());
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
}

static void debugUnits(City const& city)
{
    if (!ImGui::CollapsingHeader("Units"))
        return ;

    for (auto const& unit: city.units())
    {
        sprintf(buffer, "Unit %u (%s):", unit->id(), unit->type().c_str());
        if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextColored(COLOR(unit), "%s", unit->type().c_str());
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

            // TODO rules

            ImGui::TreePop();
        }
    }
}

static void debugMaps(City const& city)
{
    if (!ImGui::CollapsingHeader("Maps"))
        return ;

    for (auto const& it: city.maps())
    {
        Map& map = *(it.second);
        sprintf(buffer, "Map %s:", map.type().c_str());
        if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextColored(COLOR(&map), "Color");
            ImGui::TextWrapped("Capacity: %u", map.getCapacity());

            // Show resource for each cell
            if (ImGui::TreeNode("Resources:"))
            {
                for (uint32_t u = 0; u < map.gridSizeU(); ++u)
                {
                    std::stringstream ss;
                    ss << u << " =>";
                    for (uint32_t v = 0; v < map.gridSizeV(); ++v)
                    {
                        ss << " " << map.getResource(u, v);
                    }
                    ImGui::TextWrapped("%s", ss.str().c_str());
                }
                ImGui::TreePop();
            }

            // Show Map rules (if any)
            if ((map.getMapType().rules.size() != 0u) &&
                (ImGui::TreeNode("Map Rules:")))
            {
                for (auto const& irule: map.getMapType().rules)
                {
                    RuleMap const* map_rule = reinterpret_cast<RuleMap const*>(irule);
                    sprintf(buffer, "%s:", map_rule->type().c_str());
                    if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::TextWrapped("Rate: %u", map_rule->rate());
                        if (map_rule->isRandom())
                        {
                            ImGui::TextWrapped("Random: %u",
                                               map_rule->percent(100u));
                        }

                        // Show Map commands (if any)
                        for (auto const& command: map_rule->commands())
                        {
                            ImGui::TextWrapped("Command: %s", command->type().c_str());
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }
}

static void debugPaths(City const& city)
{
    if (!ImGui::CollapsingHeader("Paths"))
        return ;

    for (auto const& it: city.paths())
    {
        Path& path = *(it.second);
        sprintf(buffer, "Path %s:", path.type().c_str());
        if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen))
        {
            // List of Ways
            if (ImGui::TreeNode("Ways:"))
            {
                for (auto const& w: path.ways())
                {
                    ImGui::TextColored(COLOR(w),
                                       "Way %u: From Node %u To Node %u",
                                       w->id(), w->from().id(), w->to().id());
                }
                ImGui::TreePop();
            }

            // List of Nodes
            if (ImGui::TreeNode("Nodes:"))
            {
                for (auto const& n: path.nodes())
                {
                    ImGui::TextColored(COLOR(n), "Node %u: Position %i, %i",
                               n->id(),
                               int(n->position().x),
                               int(n->position().y));

                    // List of Ways
                    if (n->ways().size() != 0u)
                    {
                        std::stringstream ss;
                        ss << "Ways:";
                        for (auto const& w: n->ways())
                            ss << " " << w->id();
                        ImGui::BulletText("%s", ss.str().c_str());
                    }

                    // List of Units
                    if (n->units().size() != 0u)
                    {
                        std::stringstream ss;
                        ss << "Units:";
                        for (auto const& u: n->units())
                            ss << " " << u->id();
                        ImGui::BulletText("%s", ss.str().c_str());
                    }
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }
}

void debugCity(City const& city)
{
    debugAgents(city);
    debugUnits(city);
    debugMaps(city);
    debugPaths(city);
}

void debugCities(Simulation const& simulation)
{
    static int item = 0;
    std::vector<std::string> city_list;

    city_list.reserve(simulation.cities().size());
    for (auto const& city: simulation.cities())
    {
        city_list.push_back(city.second->name());
    }

    ImGui::Combo("City", &item, city_list);
    debugCity(simulation.getCity(city_list[size_t(item)]));
}
