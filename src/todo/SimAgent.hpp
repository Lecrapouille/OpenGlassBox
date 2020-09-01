using System;

struct SimAgentType
{
  std::string id;
  uint color;
  float speed;
}

class SimAgent
{
public:

  void SimAgent(SimAgentType agentType,
                int id,
                SimPoint position,
                SimUnit owner,
                SimResourceBinCollection resources,
                std::string searchTarget)
  {
    this.agentType = agentType;
    this.id = id;
    this.owner = owner;
    this.searchTarget = searchTarget;
    this.resources.AddResources(resources);
    this.worldPosition = position.worldPosition;
    this.lastPoint = position;
  }

  void Move()
  {
    if (nextPoint == nullptr)
      {
        if (UnloadResources())
          {
            lastPoint.path.box.RemoveAgent(this);
          }
        else
          {
            FindNextPoint();
          }
      }
    else
      {
        MoveTowardsNextPoint();
      }
  }

  void MoveTowardsNextPoint()
  {
    float direction;

    if (nextPoint == currentPosition.segment.point2)
      direction = 1.0f; //moving from point1 to point2
    else
      direction = -1.0f; //moving from point2 to point1

    currentPosition.offset += direction * (agentType.speed / Simulation.TICKS_PER_SECOND) / currentPosition.segment.length;
    if (currentPosition.offset < 0.0f)
      {
        currentPosition.offset = 0.0f;

        //Reached point1
        lastPoint = currentPosition.segment.point1;
        nextPoint = nullptr;
      }
    else if (currentPosition.offset > 1.0f)
      {
        currentPosition.offset = 1.0f;

        //Reached point2
        lastPoint = currentPosition.segment.point2;
        nextPoint = nullptr;
      }

    worldPosition = currentPosition.WorldPosition;
  }

  void FindNextPoint()
  {
    nextPoint = lastPoint.path.FindNextPoint(lastPoint, searchTarget, resources);

    if (nextPoint != nullptr)
      {
        currentPosition.segment = lastPoint.GetSegmentToPoint(nextPoint);
        if (lastPoint == currentPosition.segment.point1)
          currentPosition.offset = 0;
        else
          currentPosition.offset = 1;
      }
  }

  bool UnloadResources()
  {
    SimUnit targetUnit = lastPoint.GetUnitWithTargetAndCapacity(searchTarget, resources);

    if (targetUnit != nullptr)
      resources.TransferResourcesTo(targetUnit.resources);

    return resources.IsEmpty();
  }

public:

  SimAgentType agentType;

  int id;

  float radius;

  SimVector3 worldPosition;

  SimResourceBinCollection resources = new SimResourceBinCollection();

  SimUnit owner;

    std::string searchTarget;

private:

  SimPoint lastPoint;

  SimSegmentPosition currentPosition;

  SimPoint nextPoint;

};
