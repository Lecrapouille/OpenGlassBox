#ifndef SIMBOX_HPP
#  define IMBOX_HPP

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class ISimBoxListener
{
public:

  virtual void OnMapAdded(SimMap map) = 0;
  virtual void OnMapRemoved(SimMap map) = 0;

  virtual void OnPathAdded(SimPath path) = 0;
  virtual void OnPathRemoved(SimPath path) = 0;

  virtual void OnUnitAdded(SimUnit unit) = 0;
  virtual void OnUnitRemoved(SimUnit unit) = 0;

  virtual void OnAgentAdded(SimAgent agent) = 0;
  virtual void OnAgentRemoved(SimAgent agent) = 0;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimBoxListenerNull
  : public ISimBoxListener
{
public:

  virtual void OnMapAdded(SimMap map) override
  {}

  virtual void OnMapRemoved(SimMap map) override
  {}

  virtual void OnPathAdded(SimPath path) override
  {}

  virtual void OnPathRemoved(SimPath path) override
  {}

  virtual void OnUnitAdded(SimUnit unit) override
  {}

  virtual void OnUnitRemoved(SimUnit unit) override
  {}

  virtual void OnAgentAdded(SimAgent agent) override
  {}

  virtual void OnAgentRemoved(SimAgent agent) override
  {}
};

// -----------------------------------------------------------------------------
//! Everything that makes up a game:
//! -- Game Scripts:
//!    -- Play Area and other properties
//!    -- Unit types, Map types, Global bins
//!    -- Rule scripts
//! -- Game State
//!    -- Bin and cell values
//!    -- Unit locations
// -----------------------------------------------------------------------------
class SimBox
{
public:

  void SimBox(std::string id, SimVector3 worldPosition, Simulation simulation, uint32_t gridSizeX, uint32_t gridSizeY)
  {
    this.id = id;
    this.worldPosition = worldPosition;
    this.simulation = simulation;
    this.gridSizeX = gridSizeX;
    this.gridSizeY = gridSizeY;

    foreach(SimMapType mapType in simulation.simulationDefinition.mapTypes.Values)
      AddMap(mapType);

    foreach(SimPathType pathType in simulation.simulationDefinition.pathTypes.Values)
      AddPath(pathType);
  }

  SimMap AddMap(SimMapType mapType)
  {
    if (mapType == null)
      throw new ArgumentNullException("mapType");

    if (GetMap(mapType.id) != null)
      throw new ArgumentException("Duplicated mapType", "mapType");

    SimMap map = new SimMap();

    map.Init(mapType, this, gridSizeX, gridSizeY);

    maps.Add(map.id, map);

    boxListener.OnMapAdded(map);

    return map;
  }

  void RemoveMap(SimMap map)
  {
    maps.Remove(map.id);

    boxListener.OnMapRemoved(map);
  }

  SimMap GetMap(  std::string id)
  {
    SimMap val;
    if (maps.TryGetValue(id, out val))
      return val;

    return null;
  }

  IEnumerable<SimMap> GetMaps()
  {
    return maps.Values;
  }

  SimPath AddPath(SimPathType pathType)
  {
    if (pathType == null)
      throw new ArgumentNullException("pathType");

    if (GetPath(pathType.id) != null)
      throw new ArgumentException("Duplicated pathType", "pathType");

    SimPath path = new SimPath();

    path.Init(pathType, this);

    paths.Add(path.id, path);

    boxListener.OnPathAdded(path);

    return path;
  }

  void RemovePath(SimPath path)
  {
    paths.Remove(path.id);

    boxListener.OnPathRemoved(path);
  }

  SimPath GetPath(  std::string id)
  {
    SimPath val;
    if (paths.TryGetValue(id, out val))
      return val;

    return null;
  }

  IEnumerable<SimPath> GetPaths()
  {
    return paths.Values;
  }

  SimUnit AddUnit(SimUnitType unitType, SimSegmentPosition position)
  {
    SimPoint newPoint = position.segment.path.SplitSegment(position);

    return AddUnit(unitType, newPoint);
  }

  SimUnit AddUnit(SimUnitType unitType, SimPoint position)
  {
    SimUnit unit = new SimUnit();

    unit.Init(unitType, nextUnitId++, position);

    units.Add(unit);

    boxListener.OnUnitAdded(unit);

    return unit;
  }

  void RemoveUnit(SimUnit unit)
  {
    unit.Destroy();

    units.Remove(unit);

    boxListener.OnUnitRemoved(unit);
  }

  IEnumerable<SimUnit> GetUnits()
  {
    return units.items;
  }

  SimAgent AddAgent(SimAgentType agentType, SimPoint position, SimUnit owner, SimResourceBinCollection resources,   std::string searchTarget)
  {
    SimAgent agent = new SimAgent();

    agent.Init(agentType, nextAgentId++, position, owner, resources, searchTarget);

    agents.Add(agent);

    boxListener.OnAgentAdded(agent);

    return agent;
  }

  void RemoveAgent(SimAgent agent)
  {
    agents.Remove(agent);

    boxListener.OnAgentRemoved(agent);
  }

  IEnumerable<SimAgent> GetAgents()
  {
    return agents.items;
  }

  void Update()
  {
    //Agents move all the time
    agents.StartIterating();
    for (uint32_t i = 0; i < agents.items.Count; i++)
      agents.items[i].Move();
    agents.StopIterating();

    units.StartIterating();
    for (uint32_t i = 0; i < units.items.Count; i++)
      units.items[i].ExecuteRules();
    units.StopIterating();

    foreach(SimMap map in maps.Values)
      map.ExecuteRules();
  }

public:

  std::string id;
  Simulation simulation;
  SimVector3 worldPosition;
  uint32_t gridSizeX;
  uint32_t gridSizeY;

  SimResourceBinCollection globals = new SimResourceBinCollection();
  ISimBoxListener boxListener = new SimBoxListenerNull();

private:

  Dictionary<std::string, SimMap> maps;
  Dictionary<std::string, SimPath> paths;

  SimList<SimUnit> units;
  SimList<SimAgent> agents;

  uint32_t nextUnitId;
  uint32_t nextAgentId;
};

#endif
