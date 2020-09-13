#ifndef MAPCOORDINATESINSIDERADIUS_HPP
#define MAPCOORDINATESINSIDERADIUS_HPP

#include <vector>
#include <map>

class MapCoordinatesInsideRadius
{
public:

    MapCoordinatesInsideRadius() = default;

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

    void createCoordinates(uint32_t radius, std::vector<uint32_t> &res);
     std::vector<uint32_t>& cachedCoordinates(uint32_t radius)
    {
        static std::map<uint32_t, std::vector<uint32_t>> coordinates;
        return coordinates[radius];
    }

private:

    const uint32_t MAX_RADIUS = 255u;

    std::vector<uint32_t>* m_values;
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
