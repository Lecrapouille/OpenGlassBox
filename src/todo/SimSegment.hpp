#ifndef SIMSEGMENT_HPP
#  define SIMSEGMENT_HPP

#  include "SimPath.hpp"
#  include "SimPoint.hpp"
#  include <string>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimSegmentType
{
public:

  std::string id;
  uint32_t color;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimSegment
{
public:

  SimSegmentType segmentType;
  uint32_t id;
  SimPoint point1;
  SimPoint point2;
  SimPath path;
  float length;

  void SimSegment(SimPath const& path, SimSegmentType const& segmentType,
                  uint32_t const id, SimPoint const& point1, SimPoint const& point2)
  {
    this->path = path;
    this->segmentType = segmentType;
    this->id = id;
    this->point1 = point1;
    this->point2 = point2;

    point1.segments.push_back(*this);
    point2.segments.push_back(*this);
    UpdateLength();
  }

  void ~SimSegment()
  {}

  void ChangePoint2(SimPoint const& newPoint2)
  {
    point2.segments.Remove(this);

    point2 = newPoint2;

    point2.segments.push_back(*this);

    UpdateLength();
  }

private:

  void UpdateLength()
  {
    length = (point2.worldPosition - point1.worldPosition).Len;
  }
};

#if 0
// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimSegmentPosition
{
public:

  SimVector3 WorldPosition
  {
    get
      {
        return segment.point1.worldPosition + (segment.point2.worldPosition - segment.point1.worldPosition) * offset;
      }
  }

  void GetMapPosition(uint32_t& x, uint32_t& y)
  {
    SimVector3 worldPos = WorldPosition;

    x = worldPos.x / SimMap.GRID_SIZE;
    y = worldPos.z / SimMap.GRID_SIZE;

    if (x >= segment.path.box.gridSizeX)
      x = segment.path.box.gridSizeX - 1;

    if (y >= segment.path.box.gridSizeY)
      y = segment.path.box.gridSizeY  -1;
  }

public:

  SimSegment segment;
  float offset;
};
#endif

#endif
