#ifndef SIMMAPCOORDINATESINSIDERADIUS_HPP
#  define SIMMAPCOORDINATESINSIDERADIUS_HPP

#  include <unordered_map>
#  include <vector>
#  include <random>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class SimMapCoordinatesInsideRadius
{
public:

  SimMapCoordinatesInsideRadius(uint32_t const radius,
                                uint32_t const centerX, uint32_t const centerY,
                                uint32_t const minX, uint32_t const maxX,
                                uint32_t const minY, uint32_t const maxY,
                                bool const random)
  {
    std::vector<uint32_t>& values = cachedCoordinates[radius];
    if (values.size() == 0)
      CreateCoordinates(radius, values);

    static std::random_device rd;
    static std::mt19937 generator(rd());
    std::uniform_int_distribution<> rnd(0, values.size());
    this->startingIndex = (random) ? rnd(generator) : 0;
    this->offset = 0;
    this->centerX = centerX;
    this->centerY = centerY;
    this->minX = minX;
    this->maxX = maxX;
    this->minY = minY;
    this->maxY = maxY;
    this->radius = radius;
  }

  ~SimMapCoordinatesInsideRadius()
  {}

public:

  bool GetNextCoordinate(uint32_t& x, uint32_t& y)
  {
    std::vector<uint32_t>& values = cachedCoordinates[radius];
    while (offset < values.size())
      {
        uint32_t const val = values[(startingIndex + offset++) % values.size()];

        x = ((val >> 16) & 0xFFFF) - MAX_RADIUS;
        y = (val & 0xFFFF) - MAX_RADIUS;

        x += centerX;
        y += centerY;

        if ((x >= minX) && (x < maxX) && (y >= minY) && (y < maxY))
          return true;
      }

    x = 0;
    y = 0;

    return false;
  }

private:

  static void CreateCoordinates(uint32_t const radius, std::vector<uint32_t> points)
  {
    points.reserve(radius * radius);

    int32_t r = static_cast<int32_t>(radius);
    for (int32_t x = -r; x <= r; ++x)
      {
        for (int32_t y = -r; y <= r; ++y)
          {
            if (std::abs(x) + std::abs(y) <= r)
              {
                uint32_t point = ((static_cast<uint32_t>(x) + MAX_RADIUS) << 16) |
                  (static_cast<uint32_t>(y) + MAX_RADIUS);
                points.push_back(point);
              }
          }
      }
  }

private:

  static uint32_t const MAX_RADIUS = 255u;
  static std::unordered_map<uint32_t, std::vector<uint32_t>> cachedCoordinates;

  uint32_t startingIndex;
  uint32_t offset;
  uint32_t centerX;
  uint32_t centerY;
  uint32_t minX;
  uint32_t maxX;
  uint32_t minY;
  uint32_t maxY;
  uint32_t radius;
};

#endif
