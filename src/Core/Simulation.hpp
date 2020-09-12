#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#  include "Core/City.hpp"
#  include "Core/Path.hpp"
#  include "Core/Unique.hpp"
#  include <string>
#  include <map>
#  include <memory>

//==============================================================================
//! \brief Entry point class managing (add, get, remove) a collection of Cities
//! and running simulation on them.
//! In this implementation Cities are not connected.
//==============================================================================
class Simulation
{
public:

    Simulation(uint32_t gridSizeX = 32u, uint32_t gridSizeY = 32u);
    void update(float const deltaTime);
    City& addCity(std::string const& id);
    City& getCity(std::string const& id);

private:

    uint32_t      m_gridSizeX;
    uint32_t      m_gridSizeY;
    float         m_time = 0.0f;
    Cities        m_cities;
};

#endif
