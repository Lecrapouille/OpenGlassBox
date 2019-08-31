#ifndef SIMUNITS_HPP
#  define SIMUNITS_HPP

#  include <string>
//#  include "SimContext.hpp"
#  include "SimResources.hpp"
#  include "SimPoint.hpp"

// -----------------------------------------------------------------------------
//! \brief
// -----------------------------------------------------------------------------
struct SimUnitType
{
  std::string id;
  uint32_t color;
  //SimVector3 halfSize;
  int32_t mapRadius;
  SimResourceBinCollection caps;
  SimResourceBinCollection resources;
  //std::vector<SimRuleUnit*> rules;
  std::vector<std::string> targets;
};

// -----------------------------------------------------------------------------
//! \brief Represent things: houses, factories, even people.
//! A unit has state: A collection of resource bins
//! Also a well-defined spatial extent: Bounding volume, Simulation footprint.
// -----------------------------------------------------------------------------
class SimUnit
{
public:

  SimUnit(SimUnitType const& unitType, uint32_t const id, SimPoint const& position)
  {
    this->unitType = unitType;
    this->id = id;
    this->position = position;

    position.units.Add(this);

    resources.SetCapacities(unitType.caps);
    resources.AddResources(unitType.resources);

    context.localResources = resources;
    context.unit = this;
    context.box = position.path.box;
    context.globalResources = context.box.globals;
    context.mapPositionRadius = unitType.mapRadius;
  }

  ~SimUnit()
  {
    position.units.Remove(this);
  }

  void ExecuteRules()
  {
    ticks++;

    SimRule[] rules = unitType.rules;

    position.GetMapPosition(out context.mapPositionX, out context.mapPositionY);

    for (int i = 0; i < rules.size(); ++i)
      if (ticks % rules[i]->rate == 0)
        rules[i]->Execute(context);
  }

  bool Accepts(std::string const& searchTarget, SimResourceBinCollection const& resourcesToTryToAdd)
  {
    return unitType.targets.find(searchTarget) != unitType.targets.end() &&
      resources.CanAddSomeResources(resourcesToTryToAdd);
  }

public:

  SimUnitType unitType;
  uint32_t id;
  SimPoint position;
  SimResourceBinCollection resources;

private:

  SimRuleContext context;
  int ticks;
};

#endif
