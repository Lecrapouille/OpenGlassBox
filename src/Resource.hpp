#ifndef RESOURCES_HPP
#  define RESOURCES_HPP

#  include <string>
#  include <limits>
#  include <vector>

//==============================================================================
//! \brief The basic currency of the game defined by its type, amount and
//! capacity.
//!
//! A game Unit can hold an amount of resource R which cannot exceed a
//! fixed-size capacity C. Example of resource:
//!  - Citizen, cars, goods ...
//!  - Oil, coal, wood, water, electricity ...
//!  - Money, food, labour, pollution, trash ...
//!  - Happiness, sickness, taxes.
//==============================================================================
class Resource
{
public:

    using Type = std::string;

public:

    //! \brief Start with zero amount of resource but with the greater capacity
    //! possible.
    Resource(Resource::Type const& type);

    //! \brief Increase of a given quantity the current amount of resource.
    //! The result is limited by the capacity.
    void add(uint32_t const toAdd);

    //! \brief Decrease of a given quantity the current amount of resource.
    //! The result can be negative and is therefore limited to 0.
    void remove(uint32_t const toRemove);

    //! \brief Transfer resources to a given recipient.  The quantity of
    //! resources transfered is limited by the capacity of the recipient.
    void transferTo(Resource& target);

    //! \brief Modify the capacity
    void setCapacity(uint32_t const capacity);

    //! \brief Return the type of resource (ie food, sickness ...)
    inline std::string const& type() const
    {
        return m_type;
    }

    //! \brief Return how many resources can be hold.
    inline uint32_t capacity() const
    {
        return m_capacity;
    }

    //! \brief Return the current quantity of resource.
    inline uint32_t amount() const
    {
        return m_amount;
    }

    //! \brief Check if the current quantity is not null.
    inline bool hasAmount() const
    {
        return m_amount > 0u;
    }

public:

    static const uint32_t MAX_CAPACITY;

protected:

    Type     m_type;
    uint32_t m_capacity = MAX_CAPACITY;
    uint32_t m_amount = 0u;
};

#endif
