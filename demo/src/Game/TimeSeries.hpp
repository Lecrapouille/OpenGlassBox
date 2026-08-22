//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file TimeSeries.hpp
//! \brief Named time series sampled from the simulation for chart panels.


#ifndef OPEN_GLASSBOX_DEMO_TIME_SERIES_HPP
#  define OPEN_GLASSBOX_DEMO_TIME_SERIES_HPP

#  include <cstdint>
#  include <string>
#  include <vector>

namespace ogb {
namespace game {


// ****************************************************************************
//! \brief A bounded history of one scalar quantity, laid out so that it can be
//! handed to ImPlot without a copy.
//!
//! Samples are kept in two parallel arrays holding the abscissa (the tick) and
//! the ordinate. Once the capacity is reached the oldest half is dropped in one
//! go rather than shifting at every sample, which keeps the amortized cost
//! constant while leaving the data contiguous.
// ****************************************************************************
class TimeSeries
{
public:

    //! \brief Number of samples kept before the history is halved.
    static constexpr size_t CAPACITY = 2048u;

    explicit TimeSeries(std::string name = {})
        : m_name(std::move(name))
    {
        m_ticks.reserve(CAPACITY);
        m_values.reserve(CAPACITY);
    }

    // ------------------------------------------------------------------------
    //! \brief Append one sample.
    // ------------------------------------------------------------------------
    void push(uint64_t tick, float value)
    {
        pushHours(float(tick), value);
    }

    void pushHours(float hours, float value)
    {
        if (m_ticks.size() >= CAPACITY)
        {
            size_t const keep = CAPACITY / 2u;
            m_ticks.erase(m_ticks.begin(), m_ticks.end() - long(keep));
            m_values.erase(m_values.begin(), m_values.end() - long(keep));
        }

        m_ticks.push_back(hours);
        m_values.push_back(value);
    }

    void clear()
    {
        m_ticks.clear();
        m_values.clear();
    }

    std::string const& name() const { return m_name; }
    size_t size() const { return m_values.size(); }
    bool empty() const { return m_values.empty(); }
    //! \brief Same element type as values() so that both can be handed to
    //! ImPlot::PlotLine, whose two arrays must share the same type.
    float const* ticks() const { return m_ticks.data(); }
    float const* hours() const { return m_ticks.data(); }
    float const* values() const { return m_values.data(); }
    float last() const { return m_values.empty() ? 0.0f : m_values.back(); }

private:

    std::string m_name;
    std::vector<float> m_ticks;
    std::vector<float> m_values;
};
} // namespace game
} // namespace ogb

#endif
