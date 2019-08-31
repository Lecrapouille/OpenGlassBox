#ifndef SIMRULES_HPP
#  define SIMRULES_HPP

#  include <string>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimRuleEvent
{
  std::string key;
  std::string parameter;
}

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimRuleFactory {};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimRuleContext
{
  SimBox box;
  SimUnit unit;
  SimResourceBinCollection localResources;
  SimResourceBinCollection globalResources;
  uint32_t mapPositionX;
  uint32_t mapPositionY;
  uint32_t mapPositionRadius;
};

class SimRule
{
public:

  virtual bool Execute(SimRuleContext& context)
  {
    for (uint32_t i = 0; i < commands.size(); i++)
      if (!commands[i].Validate(context))
        return false;

    for (uint32_t i = 0; i < commands.size(); i++)
      commands[i].Execute(context);

    return true;
  }

  virtual void SetOption(std::string const& optionId, std::string const& val)
  {
    switch (optionId)
      {
      case "rate":
        if (!TryParse<uint32_t>(val, rate))
          rate = 1u;
        break;
      }
  }

public:

  std::string id;
  uint32_t rate = 1u;
  std::vector<SimRuleCommand*> commands;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimRuleMap
  : public SimRule
{
  bool randomTiles = false;
  int randomTilesPercent = 10;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimRuleUnit
  : public SimRule
{
public:

  virtual bool Execute(SimRuleContext context) override
  {
    return (base.Execute(context)) ? true: onFail.Execute(context);
  }

public:

  SimRuleUnit onFail;
};

#endif
