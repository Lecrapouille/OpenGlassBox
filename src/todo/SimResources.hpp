#ifndef SIMRESOURCES_HPP
#  define SIMRESOURCES_HPP

#  include <string>
#  include <limits>
#  include <vector>

static constexpr uint32_t MaxCapacity = std::numeric_limits<uint32_t>::max();
class SimResourceBin;

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimResourceType
{
  std::string id;
};

// -----------------------------------------------------------------------------
//! \brief The basic currency of the game:
//! -- Oil, coal, crops, wood, water ...
//! -- Money, electricity, labour, pollution
// -----------------------------------------------------------------------------
class SimResource
{
public:

  SimResource(SimResourceType const& resourceType)
  {
    this->resourceType = resourceType;
    this->amount = amount;
    this->capacity = capacity;
  }

  void Add(uint32_t toAdd)
  {
    amount += toAdd; // Fixme integer overflow
    if (amount > capacity)
      amount = capacity;
  }

  void Remove(uint32_t toRemove)
  {
    if (amount > toRemove)
      amount -= toRemove;
    else
      amount = 0;
  }

  void TransferTo(SimResource& targetBin)
  {
    uint32_t toTransfer =
      std::min(amount, targetBin.capacity - targetBin.amount);

    Remove(toTransfer);
    targetBin.Add(toTransfer);
  }

  void SetCapacity(uint32_t const capacity)
  {
    this->capacity = capacity;
    if (amount > capacity)
      amount = capacity;
  }

public:

  SimResourceType resourceType;
  uint32_t capacity = MaxCapacity;
  uint32_t amount = 0;
};

// -----------------------------------------------------------------------------
//! \brief Resources come in bin.
//! Bin of resource R, has capacity C
//! Bin value is an integer: 0..C
//! Capacity is fixed
// -----------------------------------------------------------------------------
class SimResourceBin
{
public:

  SimResourceBin()
  {}

  ~SimResourceBin()
  {
    uint32_t i = bin.size();
    while (i--)
      {
        delete bin[i];
      }
  }

  void AddResource(SimResourceType const& resourceType,
                   uint32_t const amount)
  {
    SimResource& res = FindOrAddResource(resourceType);
    res.Add(amount);
  }

  void AddResources(SimResourceBin const& resourcesToAdd)
  {
    for (auto const& it: resourcesToAdd.bin)
      {
        AddResource(it->resourceType, it->amount);
      }
  }

  bool CanAddSomeResources(SimResourceBin const& resourcesToTryAdd)
  {
    for (auto& it: resourcesToTryAdd.bin)
      {
        if ((it->amount > 0) &&
            (GetAmount(it->resourceType) < GetCapacity(it->resourceType)))
          {
            return true;
          }
      }

    return false;
  }

  void TransferResourcesTo(SimResourceBin& resourcesTarget)
  {
    for (auto& it: bin)
      {
        it->TransferTo(resourcesTarget.FindOrAddResource(it->resourceType));
      }
  }

  void RemoveResource(SimResourceType const& resourceType, uint32_t const amount)
  {
    SimResource* res = FindResource(resourceType);

    if (res != nullptr)
      res->Remove(amount);
  }

  uint32_t GetAmount(SimResourceType const& resourceType)
  {
    SimResource* res = FindResource(resourceType);

    return (res != nullptr) ? res->amount : 0;
  }

  void SetCapacity(SimResourceType const& resourceType, uint32_t const capacity)
  {
    SimResource& res = FindOrAddResource(resourceType);

    res.SetCapacity(capacity);
  }

  void SetCapacities(SimResourceBin const& resourcesCapacities)
  {
    for (auto& it: resourcesCapacities.bin)
      {
        SetCapacity(it->resourceType, it->amount);
      }
  }

  uint32_t GetCapacity(SimResourceType const& resourceType)
  {
    SimResource* b = FindResource(resourceType);

    return (b != nullptr) ? b->capacity : MaxCapacity;
  }

  bool IsEmpty() const
  {
    for (auto const& it: bin)
      {
        if (it->amount > 0)
          return false;
      }

    return true;
  }

private:

  SimResource* FindResource(SimResourceType const& resourceType)
  {
    for (auto& it: bin)
      {
        if (it->resourceType.id == resourceType.id)
          return it;
      }

    return nullptr;
  }

  SimResource& FindOrAddResource(SimResourceType const& resourceType)
  {
    SimResource* b = FindResource(resourceType);

    if (b == nullptr)
      {
        b = new SimResource(resourceType);
        bin.push_back(b);
      }

    return *b;
  }

public:

  std::vector<SimResource*> bin;
};

#endif
