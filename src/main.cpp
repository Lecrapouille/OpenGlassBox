#include "main.hpp"

GlassBox::GlassBox()
    : m_simulation(32u, 32u)
{}

bool GlassBox::onInit()
{
    // TODO m_simulation.load("simulation.fth");
    // TODO m_simulation.getSegmentType("Dirt")
    ResourceType resource_type[3] = { "Water", "Grass", "People" };
    PathType path_type("Road"/*, 0xAAAAAA*/);
    SegmentType segment_type("Dirt", 0xAAAAAA);
    AgentType agent_type[2] = { { "Worker", 0xFFFF00, 10 }, { "People", 0xFFFF00, 10 } };
    Resources r; r.addResource("Water", 10); r.addResource("People", 10);
    UnitType unit_type[2] = { { "Home", 0xFF00FF, r, {}, {} }, 
                              { "Work", 0xFF00FF, r, {}, {} } };

    City& city = m_simulation.addCity("Paris");
    Path& road = city.addPath(path_type);
    Node& n1 = road.addNode(Vector3f(20.0f, 20.0f, 0.0f));
    Node& n2 = road.addNode(Vector3f(50.0f, 50.0f, 0.0f));
    Node& n3 = road.addNode(Vector3f(20.0f, 50.0f, 0.0f));
    Segment& s1 = road.addSegment(segment_type, n1, n2);
    Segment& s2 = road.addSegment(segment_type, n2, n3);
    Segment& s3 = road.addSegment(segment_type, n3, n1);

    // TODO city.AddUnit/*OnSegment*/(m_simulation.getUnitType("Home"), { s1, off=0.66f });
    Unit& u1 = city.addUnit(unit_type[0], n1);
    city.addAgent(agent_type[0], u1, r, "work");

    return true;
}

// TODO use SDL_RenderDrawLines to draw a batch of segments
void GlassBox::onPaint(SDL_Renderer& renderer, float dt)
{
    City& city = m_simulation.getCity("Paris");

    // Draw the grid

    // Draw the 1st map (todo rectangle * capacity/amount)

    // Draw Areas

    // Draw Units
    for (auto& it: city.getUnits())
    {
        SDL_SetRenderDrawColor(&renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_Rect rect;
        rect.x = int(it->position().x);
        rect.y = int(it->position().y);
        rect.w = 10; // GRID_SIZE
        rect.h = 10;
        SDL_RenderFillRect(&renderer, &rect);
    }

    for (auto& path: city.getPaths())
    {
        // Draw roads
        for (auto& it: path.second->segments()) // TODO rename to getSegments ?
        {
            SDL_SetRenderDrawColor(&renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(&renderer,
                               int(it->position1().x), int(it->position1().y),
                               int(it->position2().x), int(it->position2().y));
        }

        // Draw nodes
        for (auto& it: path.second->nodes()) // TODO rename to getNodes ?
        {
            SDL_SetRenderDrawColor(&renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_Rect rect;
            rect.x = int(it->position().x);
            rect.y = int(it->position().y);
            rect.w = 2;
            rect.h = 2;
            SDL_RenderFillRect(&renderer, &rect);
        }
    }

    // Draw agents
    for (auto& it: city.getAgents())
    {
        SDL_SetRenderDrawColor(&renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
        SDL_Rect rect;
        rect.x = int(it->position().x);
        rect.y = int(it->position().y);
        rect.w = 2; // GRID_SIZE
        rect.h = 2;
        SDL_RenderFillRect(&renderer, &rect);
    }

    // Update states of the simulation
    m_simulation.update(dt);
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
