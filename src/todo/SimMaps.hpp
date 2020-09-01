#ifndef SIMMAPS_HPP
#  define SIMMAPS_HPP

#  include "SimMapRandomCoordinates.hpp"
#  include "SimMapCoordinatesInsideRadius.hpp"
//#  include "SimRuleMap.hpp"
//#  include "SimBox.hpp"
#  include <string>
#  include <vector>

// -----------------------------------------------------------------------------
struct SimMapType
{
  std::string id;
  uint32_t color;
  uint32_t capacity;
  //std::vector<SimRuleMap> rules;
};

class SimMap;

// -----------------------------------------------------------------------------
class ISimMapListener
{
public:

  virtual void OnMapModified(SimMap& map, uint32_t const x, uint32_t const y, uint32_t const val) = 0;
};

class SimMapListenerNull: public ISimMapListener
{
  //! Dummy method
  virtual void OnMapModified(SimMap& map, uint32_t const x, uint32_t const y, uint32_t const val) override
  {
    (void) map; (void) x; (void) y; (void) val;
  }
};

// -----------------------------------------------------------------------------
//! Maps represent resources in the environment
//!   -- Coal, oil, forest
//!   -- But also air pollution, land value, desirability
//!   -- Resources are limited
//! Simple uniform size grids
//!   --  Each cell is a resource bin
//! Units interact with maps through their footprint
// -----------------------------------------------------------------------------
class SimMap
{
public:
  const uint32_t GRID_SIZE = 2;

  std::string id;
  SimMapType mapType;

  SimBox box;
  ISimMapListener* mapListener = new SimMapListenerNull();

private:

  std::vector<uint32_t> values;
  uint32_t sizeX;
  uint32_t sizeY;
  uint32_t ticks;

  SimRuleContext context;
  SimMapRandomCoordinates randomCoordinates;
  SimMapCoordinatesInsideRadius coordinatesInsideRadius;

public:

  SimMap(SimMapType const& mapType, SimBox const& box, uint32_t const sizeX, uint32_t const sizeY)
  {
    this->id = mapType.id;
    this->mapType = mapType;
    this->box = box;

    values.resize(sizeX * sizeY);

    this->sizeX = sizeX;
    this->sizeY = sizeY;

    context.box = box;
    context.mapPositionRadius = 0;
  }

  ~SimMap()
  {
    delete mapListener;
  }

  void SetValue(uint32_t const x, uint32_t const y, uint32_t val)
  {
    if (val > mapType.capacity)
      val = mapType.capacity;

    Set(x, y, val);
  }

  uint32_t GetValue(uint32_t const x, uint32_t const y)
  {
    return values[y * sizeX + x];
  }

  uint32_t GetValue(uint32_t const x, uint32_t const y, uint32_t radius)
  {
    uint32_t totalInsideRadius = 0;
    CoordinatesInsideRadius c(radius, x, y, 0, sizeX, 0, sizeY, false);

    while (c.GetNextCoordinate(&x, &y))
      totalInsideRadius += GetValue(x, y);

    return totalInsideRadius;
  }

  uint32_t GetCapacity(uint32_t const x, uint32_t const y)
  {
    return mapType.capacity; // FIXME
  }

  uint32_t GetCapacity(uint32_t const x, uint32_t const y, uint32_t const radius)
  {
    uint32_t capacityInsideRadius = 0;
    CoordinatesInsideRadius c(radius, x, y, 0, sizeX, 0, sizeY, false);

    while (c.GetNextCoordinate(&x, &y))
      capacityInsideRadius += GetCapacity(x, y);

    return capacityInsideRadius;
  }

  void Add(uint32_t const x, uint32_t const y, uint32_t const toAdd)
  {
    uint32_t val = GetValue(x, y);

    toAdd = std::min(mapType.capacity - val, toAdd);
    if (toAdd > 0)
      {
        val += toAdd;
        SetValue(x, y, val);
      }
  }

  void Add(uint32_t const x, uint32_t const y, uint32_t const radius, uint32_t const toAdd)
  {
    CoordinatesInsideRadius c(radius, x, y, 0, sizeX, 0, sizeY, true);

    uint32_t remainingToAdd = toAdd;
    while (remainingToAdd > 0 && c.GetNextCoordinate(&x, &y))
      {
        uint32_t val = GetValue(x, y);

        toAdd = std::min(mapType.capacity - val, remainingToAdd);
        if (toAdd > 0)
          {
            val += toAdd;
            remainingToAdd -= toAdd;
            SetValue(x, y, val);
          }
      }
  }

  void Remove(uint32_t const x, uint32_t const y, uint32_t const toRemove)
  {
    uint32_t val = GetValue(x, y);

    toRemove = std::min(val, toRemove);
    if (toRemove > 0)
      {
        val -= toRemove;
        SetValue(x, y, val);
      }
  }

  void Remove(uint32_t const x, uint32_t const y, uint32_t radius, uint32_t toRemove)
  {
    uint32_t remainingToRemove = toRemove;
    CoordinatesInsideRadius c(radius, x, y, 0, sizeX, 0, sizeY, true);

    while (remainingToRemove > 0 && c.GetNextCoordinate(&x, &y))
      {
        uint32_t val = GetValue(x, y);
mapType
        toRemove = std::min(val, remainingToRemove);
        if (toRemove > 0)
          {
            val -= toRemove;
            remainingToRemove -= toRemove;
            SetValue(x, y, val);
          }
      }
  }

  SimVector3 GetWorldPosition(uint32_t const x, uint32_t const y)
  {
    CheckInside(x, y);

    return SimVector3(x * GRID_SIZE, 0, y * GRID_SIZE);
  }

  void ExecuteRules()
  {
    ticks++;

    for (uint32_t i = 0; i < mapType.rules.size(); ++i)
      {
        SimRuleMap& rule = *(mapType.rules[i]);

        if (ticks % rule.rate == 0)
          continue;

        if (rule.randomTiles)
          {
            uint32_t tilesAmount = (rule.randomTilesPercent * sizeX * sizeY) / 100;

            RandomCoordinates r(sizeX, sizeY);

            for (uint32_t j = 0; j < tilesAmount; j++)
              if (r.GetNextCoordinate(&context.mapPositionX, &context.mapPositionY))
                rule.Execute(context);
          }id
        else
          {
            for (uint32_t x = 0; x < sizeX; ++x)
              {
                context.mapPositionX = x;

                for (uint32_t y = 0; y < sizeY; ++y)
                  {
                    context.mapPositionY = y;
                    rule.Execute(context);
                  }
              }
          }
      }
  }

private:

  void CheckInside(uint32_t const x, uint32_t const y)
  {
    if (x >= sizeX)
      throw new ArgumentOutOfRangeException("x", x, "Invalid X Position");

    if (y >= sizeY)
      throw new ArgumentOutOfRangeException("y", y, "Invalid Y Position");
  }

  inline void Set(uint32_t const x, uint32_t const y, unit32_t val)
  {
    if (values[y * sizeX + x] != val)
      {
        values[y * sizeX + x] = val;
        mapListener->OnMapModified(*this, x, y, val);
      }
  }
};

#endif
