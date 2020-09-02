#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#  include "Box.hpp"
#  include <string>
#  include <map>
#  include <memory>

class Simulation
{
public:

    Simulation(uint32_t gridSizeX, uint32_t gridSizeY);
    void update(float const deltaTime);
    Box& addBox(std::string const& id);
    Box& getBox(std::string const& id);

private:

    using Boxes = std::map<std::string, std::shared_ptr<Box>>;

    uint32_t      m_gridSizeX;
    uint32_t      m_gridSizeY;
    float         m_time = 0.0f;
    Boxes         m_boxes;
};

#endif
