#pragma once

#include <cstdint>
#include <vector>

class HammingEncoder {
public:
    static std::vector<uint8_t> calculateControlBits(const std::vector<uint8_t>& data);

    static bool verifyDataWithControlBits(const std::vector<uint8_t>& data, const std::vector<uint8_t>& controlBits);

    static int correctErrors(std::vector<uint8_t>& data, const std::vector<uint8_t>& controlBits);

private:
    static std::vector<bool> bytesToBits(const std::vector<uint8_t>& bytes);
    static std::vector<bool> calculateHammingControlBits(const std::vector<bool>& dataBits, size_t controlBitsCount);
    static size_t calculateRequiredBitSize(size_t dataBitCount);
    static bool calculateOverallParity(const std::vector<bool>& dataBits, const std::vector<bool>& controlBits);
    static bool getOverallParityFromControlBits(const std::vector<uint8_t>& controlBits, size_t hammingControlBitsCount);
    static void packBitsToBytes(const std::vector<bool>& bits, std::vector<uint8_t>& bytes);
};
