#include "Simulation.hpp"

int main()
{
    Simulation sim; sim.load("simulation.fth");

    Box& city = sim.addBox("Paris", Vector3f(0.0f, 0.0f, 0.0f), 32u, 32u);

    Path& road = city.getPath("Road");

    Node& p1 = road.addPoint(Vector3f(20.0f, 20.0f, 0.0f));
    Node& p2 = road.addPoint(Vector3f(50.0f, 50.0f), 0.0f);
    Node& p3 = road.addPoint(Vector3f(20.0f, 50.0f, 0.0f));

    Segment& s1 = road.addSegment(sim.getSegmentType("Dirt"), p1, p2);
    Segment& s2 = road.addSegment(sim.getSegmentType("Dirt"), p2, p3);
    Segment& s3 = road.addSegment(sim.getSegmentType("Dirt"), p3, p1);

    city.AddUnit/*OnSegment*/(sim.getUnitType("Home"), { s1, off=0.66f });

    sim.update(0.1f /* dt */);

    return 0;
}
