#ifndef SIMRESOURCES_HPP
#  define SIMRESOURCES_HPP

#  include <string>
#  include <limits>
#  include <vector>

static constexpr uint32_t MaxCapacity = std::numeric_limits<uint32_t>::max();

// -----------------------------------------------------------------------------
//! \brief The basic currency of the game:
//! Oil, coal, crops, wood, water ...
//! Money, electricity, labour, pollution
// -----------------------------------------------------------------------------
struct SimResource
{
  std::string id;
};

// -----------------------------------------------------------------------------
//! Resources come in bins.
//! Bin of resource R, has m_capacity C
//! Bin value is an integer, 0..C
//! Capacity is fixed
// -----------------------------------------------------------------------------
class SimResourceBin
{
public:

  void Add(uint32_t toAdd) // TODO integer overflow
  {
    m_amount += toAdd;
    if (m_amount > m_capacity)
      m_amount = m_capacity;
  }

  void Remove(uint32_t toRemove)
  {
    if (m_amount > toRemove)
      m_amount -= toRemove;
    else
      m_amount = 0;
  }

  void TransferTo(SimResourceBin targetBin)
  {
    uint32_t toTransfer =
      std::min(m_amount, targetBin.m_capacity - targetBin.m_amount);

    Remove(toTransfer);
    targetBin.Add(toTransfer);
  }

  void SetCapacity(uint32_t capacity)
  {
    m_capacity = capacity;
    if (m_amount > capacity)
      m_amount = capacity;
  }

public:

  SimResource m_resouce;
  uint32_t m_capacity = MaxCapacity;
  uint32_t m_amount = 0;
};

// -----------------------------------------------------------------------------
//! \brief
// -----------------------------------------------------------------------------
class SimResourceBinCollection
{
public:

  SimResourceBinCollection()
  {}

  ~SimResourceBinCollection()
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        delete m_bins[i];
      }
  }

  void AddResource(SimResource const& resource, uint32_t const amount)
  {
    SimResourceBin& bin = FindOrAddBin(resource);

    bin.Add(amount);
  }

  void AddResources(SimResourceBinCollection const& resourcesToAdd)
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        AddResource(resourcesToAdd.m_bins[i]->m_resouce,
                    resourcesToAdd.m_bins[i]->m_amount);
      }
  }

  bool CanAddSomeResources(SimResourceBinCollection const& resourcesToTryAdd)
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        SimResourceBin& sourceBin = *(resourcesToTryAdd.m_bins[i]);

        if ((sourceBin.m_amount > 0) &&
            (GetAmount(sourceBin.m_resouce) < GetCapacity(sourceBin.m_resouce)))
          {
            return true;
          }
      }

    return false;
  }

  void TransferResourcesTo(SimResourceBinCollection& resourcesTarget)
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        SimResourceBin& sourceBin = *m_bins[i];
        SimResourceBin& targetBin = resourcesTarget.FindOrAddBin(sourceBin.m_resouce);

        sourceBin.TransferTo(targetBin);
      }
  }

  void RemoveResource(SimResource const& resource, uint32_t const amount)
  {
    SimResourceBin* bin = FindBin(resource);

    if (bin != nullptr)
      bin->Remove(amount);
  }

  uint32_t GetAmount(SimResource const& resource)
  {
    SimResourceBin* bin = FindBin(resource);

    return (bin != nullptr) ? bin->m_amount : 0;
  }

  void SetCapacity(SimResource const& resource, uint32_t const capacity)
  {
    SimResourceBin& bin = FindOrAddBin(resource);

    bin.SetCapacity(capacity);
  }

  void SetCapacities(SimResourceBinCollection const& resourcesCapacities)
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        SetCapacity(resourcesCapacities.m_bins[i]->m_resouce,
                    resourcesCapacities.m_bins[i]->m_amount);
      }
  }

  uint32_t GetCapacity(SimResource const& resource)
  {
    SimResourceBin* bin = FindBin(resource);

    return (bin != nullptr) ? bin->m_capacity : MaxCapacity;
  }

  bool IsEmpty()
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        if (m_bins[i]->m_amount > 0)
          return false;
      }

    return true;
  }

private:

  SimResourceBin* FindBin(SimResource const& resource)
  {
    uint32_t i = m_bins.size();
    while (i--)
      {
        if (m_bins[i]->m_resouce.id == resource.id)
          return m_bins[i];
      }

    return nullptr;
  }

  SimResourceBin& FindOrAddBin(SimResource const& resource)
  {
    SimResourceBin* bin = FindBin(resource);

    if (bin == nullptr)
      {
        bin = new SimResourceBin();
        bin->m_resouce = resource;
        m_bins.push_back(bin);
      }

    return *bin;
  }

public:

  std::vector<SimResourceBin*> m_bins;
};

#endif
