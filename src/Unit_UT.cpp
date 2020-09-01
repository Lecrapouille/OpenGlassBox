#include "Unit.hpp"

int main()
{
    Unit::Init i;
    i.id = "Unit";
    i.color = 3;
    i.capacities.setCapacity("pepole", 8u);
    i.resources.addResource("pepole", 3u);
    i.rules.push_back(RuleUnit());
    i.targets.push_back("target");

    Unit u(i, Point());

    u.executeRules();
    u.executeRules();
    u.executeRules();
    u.executeRules();

    return 0;
}
