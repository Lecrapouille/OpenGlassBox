#ifndef SIMMAPRANDOMCOORDINATES_HPP
#  define SIMMAPRANDOMCOORDINATES_HPP

#  include <vector>
#  include <random>

class SimMapRandomCoordinates
{
public:

  SimMapRandomCoordinates(uint32_t const mapSizeX, uint32_t const mapSizeY)
  {
    Init(mapSizeX, mapSizeY);
  }

  ~SimMapRandomCoordinates()
  {}

  void Init(uint32_t const mapSizeX, uint32_t const mapSizeY)
  {
    if (lastSizeX != mapSizeX || lastSizeY != mapSizeY)
      {
        randomValues.reserve(mapSizeX * mapSizeY);
        returnedValues.reserve(mapSizeX * mapSizeY);

        lastSizeX = mapSizeX;
        lastSizeY = mapSizeY;

        for (uint32_t x = 0; x < mapSizeX; ++x)
          for (uint32_t y = 0; y < mapSizeY; ++y)
            randomValues.push_back((x << 16) | y);
      }
    else
      {
        for (uint32_t i = 0; i < returnedValues.size(); ++i)
          randomValues.push_back(returnedValues[i]);
        returnedValues.clear();
      }
  }

  bool GetNextCoordinate(uint32_t& x, uint32_t& y)
  {
    if (randomValues.size() > 0)
      {
        static std::random_device rd;
        static std::mt19937 generator(rd());
        std::uniform_int_distribution<> rnd(0, randomValues.size());
        uint32_t index = rnd(generator);
        uint32_t val = randomValues[index];

        randomValues[index] = randomValues.back();
        randomValues.back() = val;

        x = ((val >> 16) & 0xFFFF);
        y = (val & 0xFFFF);

        return true;
      }
    else
      {
        x = 0;
        y = 0;

        return false;
      }
  }

private:

  std::vector<uint32_t> randomValues;
  std::vector<uint32_t> returnedValues;

  uint32_t lastSizeX = 0;
  uint32_t lastSizeY = 0;
};

#endif
