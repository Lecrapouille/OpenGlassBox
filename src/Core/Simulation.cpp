#include "Core/Simulation.hpp"

#define MAX_ITERATIONS_PER_UPDATE 20u
#define TICKS_PER_SECOND 200.0f

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

        for (auto& it: m_cities) {
            it.second->update();
        }
    }
}

City& Simulation::addCity(std::string const& id, Vector3f position)
{
    auto ptr = std::make_unique<City>(id, position, m_gridSizeX, m_gridSizeY);
    City& city = *ptr;
    m_cities[id] = std::move(ptr);
    return city;
}

City& Simulation::getCity(std::string const& id)
{
    return *m_cities.at(id);
}
