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
//! Samples are kept in parallel arrays holding the abscissa (the tick) and the
//! ordinate. Once the capacity is reached the oldest half is dropped in one go
//! rather than shifting at every sample, which keeps the amortized cost
//! constant while leaving the data contiguous.
//!
//! A third array carries an exponential moving average of the same samples,
//! kept as they arrive:
//!
//!     I <- I + eta * (observed - I)
//!
//! the same filter Way::smoothFlow applies to traffic. It is a reading aid and
//! nothing else: the simulation never sees it, and no rule reads it. Population
//! and money move in steps as a rule fires, and a curve of steps is hard to
//! read a trend off.
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
        m_smoothed.reserve(CAPACITY);
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
            m_smoothed.erase(m_smoothed.begin(),
                             m_smoothed.end() - long(keep));
        }

        // The first sample seeds the average with its own value. Starting from
        // zero would draw a ramp that never happened.
        m_average = m_values.empty()
                        ? value
                        : (m_average + SMOOTHING * (value - m_average));

        m_ticks.push_back(hours);
        m_values.push_back(value);
        m_smoothed.push_back(m_average);
    }

    void clear()
    {
        m_ticks.clear();
        m_values.clear();
        m_smoothed.clear();
        m_average = 0.0f;
    }

    std::string const& name() const { return m_name; }
    size_t size() const { return m_values.size(); }
    bool empty() const { return m_values.empty(); }
    //! \brief Same element type as values() so that both can be handed to
    //! ImPlot::PlotLine, whose two arrays must share the same type.
    float const* ticks() const { return m_ticks.data(); }
    float const* hours() const { return m_ticks.data(); }
    float const* values() const { return m_values.data(); }
    //! \brief The exponential moving average of values(), sample for sample.
    float const* smoothed() const { return m_smoothed.data(); }
    float last() const { return m_values.empty() ? 0.0f : m_values.back(); }

private:

    //! \brief Weight of a new sample in the average. Slow enough to flatten the
    //! step a rule firing makes, quick enough to follow a rush hour.
    static constexpr float SMOOTHING = 0.1f;

    std::string m_name;
    std::vector<float> m_ticks;
    std::vector<float> m_values;
    std::vector<float> m_smoothed;
    //! \brief Running average, which is also the last entry of m_smoothed.
    float m_average = 0.0f;
};
} // namespace game
} // namespace ogb

#endif
