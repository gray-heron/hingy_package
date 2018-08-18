#pragma once
#include <random>

template <typename scalartype> class Randomizer
{
    std::mt19937 gen;

  public:
    Randomizer(uint32_t seed) { gen = std::mt19937(seed); }

    scalartype RandomScalar(scalartype min, scalartype max)
    {
        std::uniform_real_distribution<> dis(min, max);
        return (scalartype)dis(gen);
    }

    uint32_t RandomInt(uint32_t min, uint32_t max)
    {
        if (min == max)
            return min;

        std::uniform_real_distribution<> dis(min, max);
        return (int)dis(gen);
    }
};
