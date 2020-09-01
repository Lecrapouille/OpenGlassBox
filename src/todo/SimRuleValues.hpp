#ifndef SIMRULEVALUES_HPP
#  define SIMRULEVALUES_HPP

#  include "SimResource.hpp"
#  include "SimRuleContext.hpp"

// -----------------------------------------------------------------------------
class SimRuleValue
{
public:

  virtual int Get(SimRuleContext context)
  {
    return 0;
  }

  virtual int Capacity(SimRuleContext context)
  {
    return 0;
  }

  virtual void Add(SimRuleContext context, int toAdd)
  {}

  virtual void Remove(SimRuleContext context, int toRemove)
  {}
};

// -----------------------------------------------------------------------------
class SimRuleValueGlobal
  : public SimRuleValue
{
public:

  virtual int Get(SimRuleContext context) override
  {
    return context.globalResources.GetAmount(resource);
  }

  virtual int Capacity(SimRuleContext context) override
  {
    return context.globalResources.GetCapacity(resource);
  }

  virtual void Add(SimRuleContext context, int toAdd) override
  {
    context.globalResources.AddResource(resource, toAdd);
  }

  virtual void Remove(SimRuleContext context, int toRemove) override
  {
    context.globalResources.RemoveResource(resource, toRemove);
  }

private:

  SimResource resource;
};

// -----------------------------------------------------------------------------
class SimRuleValueLocal
  : public SimRuleValue
{
public:

  virtual int Get(SimRuleContext context) override
  {
    return context.localResources.GetAmount(resource);
  }

  virtual int Capacity(SimRuleContext context) override
  {
    return context.localResources.GetCapacity(resource);
  }

  virtual void Add(SimRuleContext context, int toAdd) override
  {
    context.localResources.AddResource(resource, toAdd);
  }

  virtual void Remove(SimRuleContext context, int toRemove) override
  {
    context.localResources.RemoveResource(resource, toRemove);
  }

private:

  SimResource resource;
};

// -----------------------------------------------------------------------------
class SimRuleValueMap: public SimRuleValue
{
public:

  virtual int Get(SimRuleContext context) override
  {
    return context.box.GetMap(mapId).Get(context.mapPositionX,
                                         context.mapPositionY,
                                         context.mapPositionRadius);
  }

  virtual int Capacity(SimRuleContext context) override
  {
    return context.box.GetMap(mapId).Capacity(context.mapPositionX,
                                              context.mapPositionY,
                                              context.mapPositionRadius);
  }

  virtual void Add(SimRuleContext context, int toAdd) override
  {
    context.box.GetMap(mapId).Add(context.mapPositionX,
                                  context.mapPositionY,
                                  context.mapPositionRadius,
                                  toAdd);
  }

  virtual void Remove(SimRuleContext context, int toRemove) override
  {
    context.box.GetMap(mapId).Remove(context.mapPositionX,
                                     context.mapPositionY,
                                     context.mapPositionRadius,
                                     toRemove);
  }

private:

  std::string mapId;
};

#endif
