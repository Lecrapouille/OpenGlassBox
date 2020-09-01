#include "Resource.hpp"
#include <cassert>

int main()
{
    ResourceBin house;

    house.addResource("pepole", 8u);
    assert(house.getAmount("pepole") == 8u);

    house.addResource("pepole", 8u);
    assert(house.getAmount("pepole") == 16u);

    assert(house.hasResource("pepole") == true);
    assert(house.hasResource("foo") == false);

    ResourceBin house2;
    house2.addResources(house);
    assert(house2.getAmount("pepole") == 16u);
    house.transferResourcesTo(house2);
    assert(house.getAmount("pepole") == 0u);
    assert(house2.getAmount("pepole") == 32u);

    return 0;
};
