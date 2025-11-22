#include "HammingEncoder.h"

#include <QDebug>

std::vector <uint8_t> HammingEncoder::calculateControlBits(const std::vector<uint8_t>& data) {
    size_t amountOfControlBits = calculateRequiredBitSize(data.size());

    std::vector<bool> dataBits = bytesToBits(data);
    std::vector<bool> controlBits = calculateHammingControlBits(dataBits, amountOfControlBits);

    bool overallParity = calculateOverallParity(dataBits, controlBits);
    controlBits.push_back(overallParity);

    std::vector<uint8_t> result;
    packBitsToBytes(controlBits, result);

    return result;
}

std::vector<bool> HammingEncoder::bytesToBits(const std::vector<uint8_t>& bytes) {
    std::vector<bool> bits;
    bits.reserve(bytes.size() * 8);

    for (uint8_t byte : bytes) {
        for (int i = 7; i >= 0; i--) {
            bits.push_back((byte >> i) & 1);
        }
    }

    return bits;
}

std::vector<bool> HammingEncoder::calculateHammingControlBits(const std::vector<bool>& dataBits, size_t controlBitsCount) {
    std::vector<bool> controlBits(controlBitsCount, false);

    for (size_t controlIndex = 0; controlIndex < controlBitsCount; controlIndex++) {
        size_t controlBitPosition = (1 << controlIndex);
        bool parity = false;

        for (size_t dataBitIndex = 0; dataBitIndex < dataBits.size(); dataBitIndex++) {
            size_t absolutePosition = dataBitIndex + 1;

            if (absolutePosition & controlBitPosition) {
                parity ^= dataBits[dataBitIndex];
            }
        }

        controlBits[controlIndex] = parity;
    }

    return controlBits;
}

size_t HammingEncoder::calculateRequiredBitSize(size_t dataByteCount) {
    size_t amountOfControlBits = 0;

    while ((1 << amountOfControlBits) < dataByteCount * 8 + 1) {
        amountOfControlBits++;
    }

    return amountOfControlBits;
}

bool HammingEncoder::calculateOverallParity(const std::vector<bool>& dataBits, const std::vector<bool>& controlBits) {
    bool parity = false;

    for (bool bit : dataBits) {
        parity ^= bit;
    }

    for (bool bit : controlBits) {
        parity ^= bit;
    }

    return parity;
}

bool HammingEncoder::getOverallParityFromControlBits(const std::vector<uint8_t>& controlBits, size_t hammingControlBitsCount) {
    std::vector<bool> controlBitsBool = bytesToBits(controlBits);

    if (hammingControlBitsCount < controlBitsBool.size()) {
        return controlBitsBool[hammingControlBitsCount];
    }

    return false;
}

void HammingEncoder::packBitsToBytes(const std::vector<bool>& bits, std::vector<uint8_t>& bytes) {
    bytes.clear();
    if (bits.empty()) return;

    size_t byteCount = (bits.size() + 7) / 8;       // можно добавить if для случаев когда bytes пустой или в нем уже есть данные
    bytes.resize(byteCount, 0);                     // и size брать готовым а не рассчитывать

    for (size_t i = 0; i < bits.size(); i++) {
        if (bits[i]) {
            size_t byteIndex = i / 8;
            size_t bitIndex = 7 - (i % 8);
            bytes[byteIndex] |= (1 << bitIndex);
        }
    }
}

bool HammingEncoder::verifyDataWithControlBits(const std::vector<uint8_t>& data, const std::vector<uint8_t>& controlBits) {
    std::vector<uint8_t> calculatedControlBits = calculateControlBits(data);

    if (calculatedControlBits.size() != controlBits.size()) {
        return false;
    }

    for(size_t i = 0; i < calculatedControlBits.size(); i++) {
        if(calculatedControlBits[i] != controlBits[i]) {
            return  false;
        }
    }

    return true;
}

int HammingEncoder::correctErrors(std::vector<uint8_t>& data, const std::vector<uint8_t>& controlBits) {
    if (data.empty()) {
        return -1;
    }

    if (verifyDataWithControlBits(data, controlBits)) {
        return 0;
    }

    std::vector<bool> dataBits = bytesToBits(data);
    size_t hammingControlBitsCount = calculateRequiredBitSize(data.size());

    size_t syndrome = 0;

    for (size_t controlIndex = 0; controlIndex < hammingControlBitsCount; controlIndex++) {
        size_t controlBitPosition = (1 << controlIndex);
        bool calculatedParity = false;

        for (size_t dataBitIndex = 0; dataBitIndex < dataBits.size(); dataBitIndex++) {
            size_t absolutePosition = dataBitIndex + 1;
            if (absolutePosition & controlBitPosition) {
                calculatedParity ^= dataBits[dataBitIndex];
            }
        }

        size_t byteIndex = controlIndex / 8;
        size_t bitShiftIndex = 7 - (controlIndex % 8);
        bool expectedParity = (controlBits[byteIndex] >> bitShiftIndex) & 1;

        if (calculatedParity != expectedParity) {
            syndrome |= controlBitPosition;
        }
    }

    bool expectedOverallParity = getOverallParityFromControlBits(controlBits, hammingControlBitsCount);

    std::vector<bool> receivedControlBits = bytesToBits(controlBits);
    if (receivedControlBits.size() > hammingControlBitsCount) {
        receivedControlBits.resize(hammingControlBitsCount);
    }
    bool currentOverallParity = calculateOverallParity(dataBits, receivedControlBits);

    if (syndrome == 0) {
        if (currentOverallParity == expectedOverallParity) {
            //qDebug() << "Ошибок не обнаружено";
            return 0;
        } else {
            //qDebug() << "Обнаружена двойная ошибка";
            return 2;
        }
    }
    else if (syndrome > 0 && syndrome <= dataBits.size()) {
        if (currentOverallParity != expectedOverallParity) {
            dataBits[syndrome - 1] = !dataBits[syndrome - 1];
            std::vector<uint8_t> correctedData;
            packBitsToBytes(dataBits, correctedData);

            data = std::move(correctedData);
            //qDebug() << "Исправлена одиночная ошибка. Синдром:" << syndrome << "Позиция бита:" << syndrome;
            return 1;
        }
        else {
            //qDebug() << "Обнаружена двойная ошибка. Синдром:" << syndrome;
            return 2;
        }
    }
    else {
        //qDebug() << "Обнаружена двойная ошибка. Синдром:" << syndrome;
        return 2;
    }
}
