//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Simulation.hpp"
#include "UI/Panels.hpp"
#include "UI/Theme.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace ogb
{
namespace ui
{

//! \brief Ending that marks a share granted to a service, and the largest value
//! such a share takes. A hundred is everything the service asks for.
static constexpr char const* BUDGET_SUFFIX = "Budget";
static constexpr int BUDGET_MAX = 100;
static constexpr int BUDGET_DEFAULT = 70;

//! \brief Beginning that marks a rate of taxation, and the largest rate. Twenty
//! is confiscation: the ruleset makes a district leave well before that.
static constexpr char const* TAX_PREFIX = "Tax";
static constexpr int TAX_MAX = 20;
static constexpr int TAX_DEFAULT = 9;

// ----------------------------------------------------------------------------
bool BudgetPanel::isBudget(std::string const& name)
{
    std::string const suffix(BUDGET_SUFFIX);
    // The name has to be longer than the ending itself: a resource simply
    // called Budget names no service and belongs in no row.
    if (name.size() <= suffix.size())
        return false;
    return name.compare(name.size() - suffix.size(), suffix.size(), suffix) ==
           0;
}

// ----------------------------------------------------------------------------
bool BudgetPanel::isTax(std::string const& name)
{
    std::string const prefix(TAX_PREFIX);
    if (name.size() <= prefix.size())
        return false;
    return name.compare(0u, prefix.size(), prefix) == 0;
}

// ----------------------------------------------------------------------------
//! \brief The part of the name that says which service or which kind of
//! building the dial belongs to. "PoliceBudget" reads as "Police" and
//! "TaxResidential" as "Residential", which is what a player is choosing
//! between.
// ----------------------------------------------------------------------------
static std::string dialLabel(std::string const& name)
{
    if (BudgetPanel::isBudget(name))
        return name.substr(0u, name.size() - std::string(BUDGET_SUFFIX).size());
    if (BudgetPanel::isTax(name))
        return name.substr(std::string(TAX_PREFIX).size());
    return name;
}

// ----------------------------------------------------------------------------
//! \brief Set a global to an exact value.
//!
//! A resource only knows how to add and to remove, which is right for a rule: a
//! rule that could set a value would erase what every other rule did on the
//! same tick. A dial is the opposite case, since the player is its only author,
//! so the difference is applied here.
// ----------------------------------------------------------------------------
static void setGlobal(Resources& globals, std::string const& name, int value)
{
    Resource& resource = globals.findOrAddResource(name);
    uint32_t const wanted = uint32_t(std::max(0, value));
    uint32_t const held = resource.getAmount();
    if (wanted > held)
        resource.add(wanted - held);
    else if (held > wanted)
        resource.remove(held - wanted);
}

// ----------------------------------------------------------------------------
//! \brief One dial. Returns the value it now holds.
// ----------------------------------------------------------------------------
static void drawDial(Resources& globals,
                     std::string const& name,
                     int maximum,
                     int fallback,
                     char const* format,
                     char const* help)
{
    // A new city seeds nothing, and a save written before a service existed
    // knows nothing of it either. Both read zero, which would switch the
    // service off without saying so, hence the value the dial starts at.
    Resource* const held = globals.findResource(name);
    if (held == nullptr)
    {
        setGlobal(globals, name, fallback);
    }

    int value = int(globals.getAmount(name));
    std::string const label = dialLabel(name);

    ImGui::SetNextItemWidth(-140.0f);
    if (ImGui::SliderInt(label.c_str(), &value, 0, maximum, format))
    {
        setGlobal(globals, name, value);
    }
    if (ImGui::IsItemHovered() && (help != nullptr))
    {
        ImGui::SetTooltip("%s\nGlobal: %s", help, name.c_str());
    }
}

// ----------------------------------------------------------------------------
void BudgetPanel::drawCity(City& city, Simulation& simulation)
{
    Resources& globals = city.getGlobals();

    // The names come from the script, in the order the script declared them,
    // because that is the order the author of the ruleset grouped them in.
    std::vector<std::string> budgets;
    std::vector<std::string> taxes;
    for (auto const& it : simulation.getRuleset().getDefinitions().getResources())
    {
        if (isBudget(it.first))
            budgets.push_back(it.first);
        else if (isTax(it.first))
            taxes.push_back(it.first);
    }

    if (budgets.empty() && taxes.empty())
    {
        ImGui::TextDisabled(
            "This ruleset declares no dial. A resource whose name ends in\n"
            "'%s' becomes a share granted to a service, and one whose name\n"
            "starts with '%s' becomes a rate of taxation.",
            BUDGET_SUFFIX,
            TAX_PREFIX);
        return;
    }

    ImGui::SeparatorText("Treasury");

    uint32_t const money = globals.getAmount("Money");

    // What the day cost, sampled once a game day. The engine does not record
    // which rule paid for what, so this is the whole balance of the city and
    // not a breakdown per service: taxes came in, services went out.
    Ledger& ledger = m_ledgers[city.getName()];
    uint32_t const day = simulation.getClock().getDay();
    if (!ledger.seen)
    {
        ledger.moneyAtDawn = money;
        ledger.lastDay = day;
        ledger.seen = true;
    }
    else if (day != ledger.lastDay)
    {
        ledger.previousDay = int64_t(money) - int64_t(ledger.moneyAtDawn);
        ledger.moneyAtDawn = money;
        ledger.lastDay = day;
    }

    ImGui::Text("Money: %u", money);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "What the city owns. A dial says what share of it a service is\n"
            "granted, so a full treasury and an idle service are not a\n"
            "contradiction.");
    }

    int64_t const yesterday = ledger.previousDay;
    ImGui::SameLine();
    if (yesterday > 0)
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "(+%lld / day)",
                           static_cast<long long>(yesterday));
    else if (yesterday < 0)
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "(%lld / day)",
                           static_cast<long long>(yesterday));
    else
        ImGui::TextDisabled("(balanced)");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(
            "What the previous game day left, taxes in and services out.\n"
            "A city that loses money every day switches its services off on\n"
            "its own once the treasury empties: a payment a rule cannot make\n"
            "refuses, and the Rule Log names it.");
    }

    if (!budgets.empty())
    {
        ImGui::SeparatorText("Granted to the services");
        ImGui::TextDisabled("Share of what each service asks for.");
        for (std::string const& name : budgets)
        {
            drawDial(globals,
                     name,
                     BUDGET_MAX,
                     BUDGET_DEFAULT,
                     "%d %%",
                     "What share of its asking price this service is granted.\n"
                     "At nothing it does not run at all, however full the\n"
                     "treasury. A ruleset grades the effect in steps, so a\n"
                     "small share buys a small effect at a small cost.");
        }
    }

    if (!taxes.empty())
    {
        ImGui::SeparatorText("Rates of taxation");
        ImGui::TextDisabled("What each kind of building pays.");
        for (std::string const& name : taxes)
        {
            drawDial(globals,
                     name,
                     TAX_MAX,
                     TAX_DEFAULT,
                     "%d",
                     "What this kind of building pays the city. A high rate\n"
                     "brings in more and costs the district its desirability,\n"
                     "so over-taxing pays for the city and empties it.");
        }
    }
}

// ----------------------------------------------------------------------------
void BudgetPanel::draw(Simulation& simulation)
{
    if (!ImGui::Begin("Budget"))
    {
        ImGui::End();
        return;
    }

    if (simulation.getCities().empty())
    {
        ImGui::TextDisabled("No city in the simulation.");
        ImGui::End();
        return;
    }

    bool first = true;
    for (auto& it : simulation.getCities())
    {
        City& city = *it.second;
        // One city is the usual case, and naming it then only adds noise.
        if (!first || (simulation.getCities().size() > 1u))
            ImGui::SeparatorText(city.getName().c_str());
        drawCity(city, simulation);
        first = false;
    }

    ImGui::End();
}
} // namespace ui
} // namespace ogb
