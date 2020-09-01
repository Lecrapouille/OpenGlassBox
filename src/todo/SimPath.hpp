#ifndef SIMPATH_HPP
#  define SIMPATH_HPP

#  include "SimBox.hpp"
#  include <string>
#  include <vector>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class ISimPathListener
{
public:

  virtual void OnSegmentAdded(SimPath const& path, SimSegment const& segment) = 0;
  virtual void OnSegmentRemoved(SimPath const& path, SimSegment const& segment) = 0;
  virtual void OnSegmentModified(SimPath const& path, SimSegment const& segment) = 0;

  virtual void OnPointAdded(SimPath const& path, SimPoint point) = 0;
  virtual void OnPointRemoved(SimPath const& path, SimPoint point) = 0;
  virtual void OnPointModified(SimPath const& path, SimPoint point) = 0;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimPathListenerNull
  : public ISimPathListener
{
public:

  virtual void OnSegmentAdded(SimPath const&, SimSegment const&) override
  {}

  virtual void OnSegmentModified(SimPath const&, SimSegment const&) override
  {}

  virtual void OnSegmentRemoved(SimPath const&, SimSegment const&) override
  {}

  virtual void OnPointAdded(SimPath const&, SimPoint const&) override
  {}

  virtual void OnPointRemoved(SimPath const&, SimPoint const&) override
  {}

  virtual void OnPointModified(SimPath const&, SimPoint const&) override
  {}
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct SimPathType
{
  std::string id;
  uint32_t color;
};

// -----------------------------------------------------------------------------
//! Points connected by Segments make up.
//! Paths make up Path Sets.
//! Fully 3D, spline-based, rich set of operations.
//! Typically player created.
//! Curvy roads!
//! But also: power lines, water pipes, flight paths
// -----------------------------------------------------------------------------
class SimPath
{
public:

  void SimPath(SimPathType const& pathType, SimBox const& box)
  {
    this.pathType = pathType;
    this.box = box;
    this.id = pathType.id;
  }

  SimPoint& AddPoint(SimVector3 const& worldPosition)
  {
    SimPoint point(*this, nextPointId++, worldPosition);

    points.push_back(point);
    pathListener.OnPointAdded(*this, point);

    return points.back();
  }

  SimSegment& AddSegment(SimSegmentType const& segmentType, SimPoint const& p1, SimPoint const& p2)
  {
    SimSegment const& segment(*this, segmentType, nextSegmentId++, p1, p2);

    segments.push_back(segment);
    pathListener.OnSegmentAdded(*this, segment);

    return segments.push_back();
  }

#if 0
  void RemovePoint(SimPoint const& point)
  {
    point.Destroy();

    points.Remove(point);

    pathListener.OnPointRemoved(*this, point);
  }

  void RemoveSegment(SimSegment const& segment)
  {
    segment.Destroy();

    segments.Remove(segment);

    pathListener.OnSegmentRemoved(*this, segment);
  }

  SimPoint SplitSegment(SimSegmentPosition const& positionToSplit)
  {
    if (positionToSplit.offset == 0)
      return positionToSplit.segment.point1;
    else if (positionToSplit.offset == 1)
      return positionToSplit.segment.point2;

    SimPoint newPoint = AddPoint(positionToSplit.WorldPosition);

    AddSegment(positionToSplit.segment.segmentType, newPoint, positionToSplit.segment.point2);

    positionToSplit.segment.ChangePoint2(newPoint);

    pathListener.OnSegmentModified(*this, positionToSplit.segment);

    return newPoint;
  }

  SimPoint FindNextPoint(SimPoint fromPoint, string searchTarget, SimResourceBinCollection resources)
  {
    //This implementation MUST be replaced with something that is really fast.. right now
    //we are not even using a priority queue!!
    //We should store as much information as possible in each SimPoint to make this search as
    //fast as possible.
    closedSet.Clear();
    openSet.Clear();
    cameFrom.Clear();
    scoreFromStart.Clear();
    scorePlusHeuristicFromStart.Clear();

    openSet.push_back(fromPoint);
    scoreFromStart[fromPoint] = 0;
    scorePlusHeuristicFromStart[fromPoint] = scoreFromStart[fromPoint] + Heuristic(fromPoint, fromPoint);

    while (openSet.Count > 0)
      {
        SimPoint current = GetPointWithLowestScorePlusHeuristicFromStart();

        if (current.GetUnitWithTargetAndCapacity(searchTarget, resources) != null)
          {
            if (current == fromPoint)
              return current;

            while(cameFrom[current] != fromPoint)
              current = cameFrom[current];

            return current;
          }

        openSet.Remove(current);
        closedSet.push_back(current);

        foreach(SimSegment const& segment in current.segments)
          {
            SimPoint neighbor;
            if (segment.point1 == current)
              neighbor = segment.point2;
            else
              neighbor = segment.point1;

            float neighborScoreFromStart = scoreFromStart[current] + segment.length;

            if (closedSet.Contains(neighbor))
              if (neighborScoreFromStart >= scoreFromStart[neighbor])
                continue;

            if (!openSet.Contains(neighbor) || neighborScoreFromStart < scoreFromStart[neighbor])
              {
                cameFrom[neighbor] = current;
                scoreFromStart[neighbor] = neighborScoreFromStart;
                scorePlusHeuristicFromStart[neighbor] = neighborScoreFromStart + Heuristic(neighbor, fromPoint);
                if (!openSet.Contains(neighbor))
                  openSet.push_back(neighbor);
              }
          }
      }

    //No path found.. return random point!
    if (fromPoint.segments.Count > 0)
      {
        SimSegment randomSegment = fromPoint.segments[rnd.Next(0, fromPoint.segments.Count)];

        if (randomSegment.point1 == fromPoint)
          return randomSegment.point2;
        else if (randomSegment.point2 == fromPoint)
          return randomSegment.point1;
      }

    return null;
  }

  SimPoint GetPointWithLowestScorePlusHeuristicFromStart()
  {
    float lowestValue = float.MaxValue;
    SimPoint lowestPoint = null;

    foreach(KeyValuePair<SimPoint, float> entry in scorePlusHeuristicFromStart)
      {
        if (entry.Value < lowestValue)
          {
            lowestValue = entry.Value;
            lowestPoint = entry.Key;
          }
      }

    if (lowestPoint != null)
      scorePlusHeuristicFromStart.Remove(lowestPoint);

    return lowestPoint;
  }

  float Heuristic(SimPoint p1, SimPoint p2)
  {
    return (p2.worldPosition - p1.worldPosition).Len;
  }
#endif

public:

  std::string id;
  SimPathType pathType;
  SimBox box;

  std::vector<SimPoint*> points;
  std::vector<SimSegment*> segments;

  pISimPathListener pathListener;

private:

  uint32_t nextPointId;
  uint32_t nextSegmentId;

#if 0
  static HashSet<SimPoint> closedSet;
  static std::vector<SimPoint> openSet;
  static Dictionary<SimPoint, SimPoint> cameFrom;
  static Dictionary<SimPoint, float> scoreFromStart;
  static Dictionary<SimPoint, float> scorePlusHeuristicFromStart;
#endif
};

#endif
