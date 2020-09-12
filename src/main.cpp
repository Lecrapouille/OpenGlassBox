#include "main.hpp"

GlassBox::GlassBox()
    : m_simulation(32u, 32u)
{}

bool GlassBox::onInit()
{
    // TODO
    // m_simulation.load("simulation.fth");

    City& city = m_simulation.addCity("Paris");
    Path& road = city.getPath("Road");

    Node& n1 = road.addNode(Vector3f(20.0f, 20.0f, 0.0f));
    Node& n2 = road.addNode(Vector3f(50.0f, 50.0f, 0.0f));
    Node& n3 = road.addNode(Vector3f(20.0f, 50.0f, 0.0f));

    Segment& s1 = road.addSegment(n1, n2);

#if 0
    Segment& s1 = road.addSegment(m_simulation.getSegmentType("Dirt"), p1, p2); // getSegmentType defini lors du parsing du script
    Segment& s2 = road.addSegment(m_simulation.getSegment("Dirt"), p2, p3);
    Segment& s3 = road.addSegment(m_simulation.getSegment("Dirt"), p3, p1);

    city.AddUnit/*OnSegment*/(m_simulation.getUnitType("Home"), { s1, off=0.66f });
#endif

    return true;
}

void GlassBox::onPaint(SDL_Renderer& renderer, float dt)
{
    //City& city = m_simulation.getCity("Paris");
    //Path& road = city.getPath("Road");

    // TODO use SDL_RenderDrawLines to draw a batch of segments
    for (auto& it: road.segments())
    {
        SDL_SetRenderDrawColor(&renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(&renderer, int(it->position1().x), int(it->position1().y),
                           int(it->position2().x), int(it->position2().y));
    }

    //m_simulation.update(dt);
}

void GlassBox::onRelease()
{
    // Do nothing
}

void GlassBox::onKeyDown(int key)
{
    switch (key)
    {
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
        printf("Touche appuyee\n");
        break;
    }
}

int main()
{
    Window w;
    GlassBox game;

    w.color(100, 100, 100);
    return w.run(game) ? EXIT_SUCCESS : EXIT_FAILURE;
}
