#include "Simulation.hpp"

int main()
{
    Simulation sim(32u, 32u); //TODO sim.load("simulation.fth");

    City& city = sim.addCity("Paris"/*, Vector3f(0.0f, 0.0f, 0.0f)*/);

    Path& road = city.getPath("Road");

    Node& p1 = road.addNode(Vector3f(20.0f, 20.0f, 0.0f));
    Node& p2 = road.addNode(Vector3f(50.0f, 50.0f, 0.0f));
    Node& p3 = road.addNode(Vector3f(20.0f, 50.0f, 0.0f));
#if 0
    Segment& s1 = road.addSegment(sim.getSegmentType("Dirt"), p1, p2); // getSegmentType defini lors du parsing du script
    Segment& s2 = road.addSegment(sim.getSegment("Dirt"), p2, p3);
    Segment& s3 = road.addSegment(sim.getSegment("Dirt"), p3, p1);

    city.AddUnit/*OnSegment*/(sim.getUnitType("Home"), { s1, off=0.66f });
#endif
    sim.update(0.1f /* dt */);

    return 0;
}
