#ifndef MAPCOORDINATESINSIDERADIUS_HPP
#define MAPCOORDINATESINSIDERADIUS_HPP

#include <vector>
#include <map>

class MapCoordinatesInsideRadius
{
public:

    using Coordinates = std::vector<int32_t>;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    MapCoordinatesInsideRadius() = default;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void init(uint32_t radius,
              uint32_t centerU, uint32_t centerV,
              uint32_t minU, uint32_t maxU,
              uint32_t minV, uint32_t maxV,
              bool random);

    //--------------------------------------------------------------------------
    //! \brief Get the next coordinate
    //! \param[out] u: the next coordinate on the U-axis.
    //! \param[out] v: the next coordinate on the V-axis.
    //! \return true if has a next coordinate, else return false.
    //--------------------------------------------------------------------------
    bool next(uint32_t& u, uint32_t& v);

private:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void createCoordinates(int32_t radius, Coordinates &res);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    static Coordinates& cachedCoordinates(uint32_t radius)
    {
        static std::map<uint32_t, Coordinates> coordinates;
        return coordinates[radius];
    }

    //--------------------------------------------------------------------------
    //! \brief Compress two numbers into one number. This function only works
    //! with small values.
    //! This function is used to store axis-U and axis-V as a single value.
    //--------------------------------------------------------------------------
    static int32_t compress(int32_t u, int32_t v);

    //--------------------------------------------------------------------------
    //! \brief Decompress one number into a pair of number.
    //--------------------------------------------------------------------------
    static void uncompress(int32_t val, int32_t& u, int32_t& v);

private:

    static constexpr int32_t MAX_RADIUS = 255;

    Coordinates* m_values;
    uint32_t m_startingIndex;
    uint32_t m_offset;
    uint32_t m_centerU;
    uint32_t m_centerV;
    uint32_t m_minU;
    uint32_t m_maxU;
    uint32_t m_minV;
    uint32_t m_maxV;
};

#endif
