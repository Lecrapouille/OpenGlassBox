#ifndef SIMULATION_HPP
#  define SIMULATION_HPP

#  include <string>
#  include <unordered_map>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class ISimulationListener
{
public:

  virtual void OnBoxAdded(SimBox box) = 0;
  virtual void OnBoxRemoved(SimBox box) = 0;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimulationListenerNull
  : public ISimulationListener
{
public:

  virtual void OnBoxAdded(SimBox box) override
  {}

  virtual void OnBoxRemoved(SimBox box) override
  {}
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class Simulation
{
public:

  Simulation()
  {
    simulationListener = new SimulationListenerNull();
  }

  ~Simulation()
  {
    delete simulationListener;
  }

  SimBox& AddBox(std::string const& id, SimVector3 center, uint32_t gridSizeX, uint32_t gridSizeY)
  {
    if (id.empty())
      throw new ArgumentNullException("id");

    if (!TryGetBox(id))
      throw new ArgumentException("Duplicated id", "id");

    SimBox* box = new SimBox(id, center, this, gridSizeX, gridSizeY);
    boxes.Add(box);
    simulationListener.OnBoxAdded(*box);

    return *box;
  }

  void RemoveBox(SimBox& box)
  {
    if (boxes.Remove(box))
      simulationListener.OnBoxRemoved(box);
  }

  bool TryGetBox(std::string const& id, SimBox& box)
  {
    for (auto& it: boxes)
      {
        if (it.id == id)
          {
            box = it;
            return true;
          }
      }

    return false;
  }

  void Update(float const deltaTime)
  {
    time += deltaTime;

    // Rules are execute at TICKS_PER_SECOND intervals
    uint32_t maxIterations = MAX_ITERATIONS_PER_UPDATE;
    while ((time >= INV_TICKS_PER_SECOND) && (maxIterations-- > 0))
      {
        time -= INV_TICKS_PER_SECOND;

        for (auto& it: boxes)
          it->Update();
      }
  }

public:

  const uint32_t TICKS_PER_SECOND = 20u;
  const float_t INV_TICKS_PER_SECOND = 1.0f / static_cast<float_t>(TICKS_PER_SECOND);
  const uint32_t MAX_ITERATIONS_PER_UPDATE = 200u;
  SimulationDefinition simulationDefinition;
  std::vector<SimBox*> boxes;
  ISimulationListener* simulationListener = new SimulationListenerNull();

private:

  float time;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimulationDefinition
{
public:

  std::unordered_map<std::string, SimResource> resourceTypes;
  std::unordered_map<std::string, SimMapType> mapTypes;
  std::unordered_map<std::string, SimPathType> pathTypes;
  std::unordered_map<std::string, SimSegmentType> segmentTypes;
  std::unordered_map<std::string, SimAgentType> agentTypes;
  std::unordered_map<std::string, SimUnitType> unitTypes;

public:

  bool TryGetResource(std::string const& id, SimResource& val)
  {
    auto it = resourceTypes.find(id);
    bool res = (it != resourceTypes.end());
    if (res) val = it;
    return res;
  }

  bool TryGetMapType(std::string const& id, SimMapType& val)
  {
    auto it = mapTypes.find(id);
    bool res = (it != mapTypes.end());
    if (res) val = it;
    return res;
  }

  bool GetPathType(std::string const& id, SimPathType& val)
  {
    auto it = pathTypes.find(id);
    bool res = (it != pathTypes.end());
    if (res) val = it;
    return res;
  }

  bool GetSegmentType(std::string const& id, SimSegmentType& val)
  {
    auto it = segmentTypes.find(id);
    bool res = (it != segmentTypes.end());
    if (res) val = it;
    return res;
  }

  bool GetAgentType(std::string const& id, SimAgentType& val)
  {
    auto it = agentTypes.find(id);
    bool res = (it != agentTypes.end());
    if (res) val = it;
    return res;
  }

  bool GetUnitType(std::string const& id, SimUnitType& val)
  {
    auto it = unitTypes.find(id);
    bool res = (it != unitTypes.end());
    if (res) val = it;
    return res;
  }
};

#endif
