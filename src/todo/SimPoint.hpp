#ifndef SIMPOINT_HPP
#  define SIMPOINT_HPP

#  include "SimVector.hpp"
#  include "SimPath.hpp"
#  include "SimSegment.hpp"
#  include "SimUnit.hpp"
#  include <vector>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimPoint
{
public:

  SimPoint(SimPath const& path, uint32_t const id, SimVector3 const& worldPosition)
  {
    this.path = path;
    this.id = id;
    this.worldPosition = worldPosition;
  }

  ~SimPoint()
  {}

  void GetMapPosition(uint32_t& x, uint32_t& y)
  {
    SimVector3 worldPos = worldPosition;

    x = ((uint32_t) worldPos.x) / SimMap.GRID_SIZE;
    y = ((uint32_t) worldPos.z) / SimMap.GRID_SIZE;

    if (x >= path.box.gridSizeX)
      x = path.box.gridSizeX - 1;

    if (y >= path.box.gridSizeY)
      y = path.box.gridSizeY  -1;
  }

  bool GetSegmentToPoint(SimPoint const& point, SimSegment& res)
  {
    for (auto& it: segments)
      {
        if (it.point1 == point || it.point2 == point)
          {
            res = it;
            return true;
          }
      }

    return false;
  }

  bool GetUnitWithTargetAndCapacity(std::string const& searchTarget,
                                    SimResourceBinCollection const& resources,
                                    SimUnit& unit)
  {
    for (auto& it: units)
      {
        if (it.Accepts(searchTarget, resources))
          {
            unit = units[i];
            return false;
          }
      }
    return false;
  }

public:

  uint32_t id;
  SimVector3 worldPosition;
  SimPath path;
  std::vector<SimSegment> segments;
  std::vector<SimUnit> units;
};

#endif
