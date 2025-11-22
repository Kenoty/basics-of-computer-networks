#pragma once
#include <random>
#include <vector>

class ErrorSimulator {
public:
    static int simulateErrors(std::vector<uint8_t>& data);

private:
    static std::mt19937& getRandomGenerator();
    static int getErrorCount();
    static void flipRandomBit(std::vector<uint8_t>& data);
};
