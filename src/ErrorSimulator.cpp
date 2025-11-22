#include "ErrorSimulator.h"
#include <random>

int ErrorSimulator::simulateErrors(std::vector<uint8_t>& data) {
    if (data.empty()) return 0;

    const int errorCount = getErrorCount();

    for (int i = 0; i < errorCount; i++) {
        flipRandomBit(data);
    }

    return errorCount;
}

std::mt19937& ErrorSimulator::getRandomGenerator() {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    return generator;
}

int ErrorSimulator::getErrorCount() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double probability = dist(getRandomGenerator());

    return (probability < 0.75) ? 1 : 2;
}

void ErrorSimulator::flipRandomBit(std::vector<uint8_t>& data) {
    std::uniform_int_distribution<size_t> byteDist(0, data.size() - 1);
    std::uniform_int_distribution<int> bitDist(0, 7);

    size_t byteIndex = byteDist(getRandomGenerator());
    int bitPosition = bitDist(getRandomGenerator());

    data[byteIndex] ^= (1 << bitPosition);
}
