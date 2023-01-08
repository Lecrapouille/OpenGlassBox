//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/MapRandomCoordinates.hpp"
#include <random>
#include <iostream>

//------------------------------------------------------------------------------
// Random numbers
//------------------------------------------------------------------------------

// Will be used to obtain a seed for the random number engine
static std::random_device rd;
// Standard mersenne_twister_engine seeded with rd()
static std::mt19937 generator(rd());

// -----------------------------------------------------------------------------
void MapRandomCoordinates::init(uint32_t mapSizeU, uint32_t mapSizeV)
{
    size_t const nbCells = mapSizeU * mapSizeV;

    if (m_randomCoordinates.size() == nbCells)
    {
        // End iterating to be sure that std::vectors have the same size.
        uint32_t u, v;
        while (next(u, v))
            ;

        // Refill with previously randomized coordinates
        m_randomCoordinates = m_returnedCoordinates;
        m_returnedCoordinates.clear();
    }
    else // New dimension: recreate cached values
    {
        m_randomCoordinates.clear();
        m_randomCoordinates.resize(nbCells);
        m_returnedCoordinates.clear();
        m_returnedCoordinates.reserve(nbCells);

        uint32_t i = 0u;
        for (uint32_t u = 0u; u < mapSizeU; ++u)
        {
            for (uint32_t v = 0u; v < mapSizeV; ++v)
            {
                m_randomCoordinates[i++] = ((u << 16) | v);
            }
        }
    }
}

// -----------------------------------------------------------------------------
bool MapRandomCoordinates::next(uint32_t& u, uint32_t& v)
{
    size_t const size = m_randomCoordinates.size();
    if (size > 0u) // Has next !
    {
        // Random a candidate
        std::uniform_int_distribution<uint32_t> rnd(0u, uint32_t(size - 1u));
        uint32_t index = rnd(generator);

        // Return the randomized Map UV coordinate
        uint32_t coord = m_randomCoordinates[index];
        u = ((coord >> 16) & 0xFFFF);
        v = (coord & 0xFFFF);
        m_returnedCoordinates.push_back(coord);

        // Remove the used coordinate (swap with the last element) in the
        // list of candidates
        m_randomCoordinates[index] = m_randomCoordinates[size - 1u];
        m_randomCoordinates.pop_back();

        return true;
    }

    u = 0u;
    v = 0u;
    return false;
}
