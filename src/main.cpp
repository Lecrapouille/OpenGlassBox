#include "main.hpp"

GlassBox::GlassBox()
    : m_simulation(32u, 32u)
{}

bool GlassBox::onInit()
{
    Script script("/home/qq/MyGitHub/GlassBox/src/Simulation/TestCity2.txt");
    if (!script)
        return false;

    City& city = m_simulation.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    Path& road = city.addPath(script.getPathType("Road"));
    Node& n1 = road.addNode(Vector3f(20.0f, 20.0f, 0.0f));
    Node& n2 = road.addNode(Vector3f(50.0f, 50.0f, 0.0f));
    Node& n3 = road.addNode(Vector3f(20.0f, 50.0f, 0.0f));
    Way& w1 = road.addWay(script.getWayType("Dirt"), n1, n2);
    Way& w2 = road.addWay(script.getWayType("Dirt"), n2, n3);
    Way& w3 = road.addWay(script.getWayType("Dirt"), n3, n1);
    Unit& u1 = city.addUnit(script.getUnitType("Home"), road, w1, 0.66f);
    Unit& u2 = city.addUnit(script.getUnitType("Home"), road, w1, 0.5f);
    Unit& u3 = city.addUnit(script.getUnitType("Work"), road, w2, 0.5f);
    Unit& u4 = city.addUnit(script.getUnitType("Work"), road, w3, 0.5f);

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
        for (auto& it: path.second->ways()) // TODO rename to getWays ?
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
