#ifndef SIMRULECOMMANDS_HPP
#define SIMRULECOMMANDS_HPP

using System;

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimRuleCommand
{
public:

  virtual bool Validate(SimRuleContext context)
  {
    return true;
  }

  virtual void Execute(SimRuleContext context)
  {}
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimRuleCommandAdd
  : public SimRuleCommand
{
public:

  virtual bool Validate(SimRuleContext& context) override
  {
    return target.Get(context) < target.Capacity(context);
  }

  virtual void Execute(SimRuleContext& context) override
  {
    target.Add(context, amount);
  }

public:

  SimRuleValue target;
  uint32_t amount;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimRuleCommandAgent : SimRuleCommand
{
public:

  virtual bool Validate(SimRuleContext& context) override
  {
    return true;
  }

  virtual void Execute(SimRuleContext& context) override
  {
    context.box.AddAgent(agentType, context.unit.position,
                         context.unit, resources, searchTarget);
  }

public:

  string searchTarget;
  SimAgentType agentType;
  SimResourceBinCollection resources;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimRuleCommandRemove : SimRuleCommand
{
public:

  virtual bool Validate(SimRuleContext& context) override
  {
    return target.Get(context) >= amount;
  }

  virtual void Execute(SimRuleContext& context) override
  {
    target.Remove(context, amount);
  }

public:

  SimRuleValue target;
  uint32_t amount;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimRuleCommandTest : SimRuleCommand
{
public:

  enum Comparison
    {
     Equals,
     Greater,
     Less
    };

  uint32_t amount;
  Comparison comparison;
  SimRuleValue target;

public:

  virtual bool Validate(SimRuleContext& context) override
  {
    uint32_t val = target.Get(context);

    switch(comparison)
      {
      case Comparison.Equals:
        return val == amount;

      case Comparison.Greater:
        return val > amount;

      case Comparison.Less:
        return val < amount;
      }

    return true;
  }

  virtual void Execute(SimRuleContext& context) override
  {
    //do nothing
  }
};

#endif
