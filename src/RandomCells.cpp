//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/RandomCells.hpp"

#include <algorithm>
#include <climits>
#include <random>

namespace ogb
{

// -----------------------------------------------------------------------------
RandomCells::RandomCells()
{
    std::random_device seed;
    m_state = (uint64_t(seed()) << 32) ^ uint64_t(seed());

    // The generator stands still on a state of zero.
    if (m_state == 0u)
        m_state = 0x9E3779B97F4A7C15ULL;
}

// -----------------------------------------------------------------------------
uint32_t RandomCells::random()
{
    // xorshift64, Marsaglia. The high half is the better mixed one.
    m_state ^= m_state << 13;
    m_state ^= m_state >> 7;
    m_state ^= m_state << 17;

    return uint32_t(m_state >> 32);
}

// -----------------------------------------------------------------------------
void RandomCells::init(uint32_t const layerSizeU,
                                uint32_t const layerSizeV,
                                uint64_t const wanted)
{
    uint64_t const cells = uint64_t(layerSizeU) * uint64_t(layerSizeV);

    m_sizeU = layerSizeU;
    m_u = 0u;
    m_v = 0u;
    m_left = uint32_t(std::min<uint64_t>(cells, UINT32_MAX));
    m_wanted = uint32_t(std::min<uint64_t>(wanted, m_left));
}

// -----------------------------------------------------------------------------
bool RandomCells::next(uint32_t& u, uint32_t& v)
{
    while (m_wanted > 0u)
    {
        // Take this cell with probability m_wanted / m_left. Once as many
        // cells are left as are still wanted the draw always succeeds, so the
        // sample always comes out at the size asked for.
        bool const taken = (random(m_left) < m_wanted);

        u = m_u;
        v = m_v;

        // Advance the scan one cell along the row, wrapping to the next.
        ++m_u;
        if (m_u == m_sizeU)
        {
            m_u = 0u;
            ++m_v;
        }
        --m_left;

        if (taken)
        {
            --m_wanted;
            return true;
        }
    }

    u = 0u;
    v = 0u;
    return false;
}

} // namespace ogb
