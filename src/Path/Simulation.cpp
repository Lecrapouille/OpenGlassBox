#include "Simulation.hpp"
#include "City.hpp"

#define MAX_ITERATIONS_PER_UPDATE 20u
#define TICKS_PER_SECOND 200.0f;

Simulation::Simulation(uint32_t gridSizeX, uint32_t gridSizeY)
    : m_gridSizeX(gridSizeX),
      m_gridSizeY(gridSizeY)
{}

void Simulation::update(float const deltaTime)
{
    m_time += deltaTime;

    // Rules are execute at TICKS_PER_SECOND intervals
    uint32_t maxIterations = MAX_ITERATIONS_PER_UPDATE;
    while ((m_time >= 1.0f / TICKS_PER_SECOND) && (maxIterations-- > 0u))
    {
        m_time -= 1.0f / TICKS_PER_SECOND;

        uint32_t t = m_boxes.size();
        while (i--) {
            m_boxes[i].update();
        }
    }
}

City& Simulation::addCity(std::string const& id)
{
    std::make_shared<City> box =
            std::make_shared<City>(id, m_gridSizeX, m_gridSizeY);
    m_boxes[id] = box;
    return *box;
}

City& Simulation::getCity(std::string const& id)
{
    return *m_boxes.at(id);
}
