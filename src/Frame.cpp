#include "Frame.h"
#include "ErrorSimulator.h"

void Frame::setData(const std::vector<uint8_t>&data) {
    this->data = data;
}

void Frame::setData(const std::string& data) {
    this->data.assign(data.begin(), data.end());
}

std::string Frame::dataToString() const {
    return std::string(this->data.begin(), this->data.end());
}

std::vector<uint8_t> Frame::serialize() const {
    std::vector<uint8_t> bytes;

    bytes.push_back(this->startFlag);
    bytes.push_back(this->total);
    bytes.push_back(this->sequence);

    bytes.insert(bytes.end(), data.begin(), data.end());

    bytes.insert(bytes.end(), fcs.begin(), fcs.end());

    bytes.push_back(this->endFlag);

    return bytes;
}

bool Frame::deserialize(const std::vector<uint8_t>& data) {

    if(data.size() < HEADER_SIZE + 1 + fcs.size() + TRAILER_SIZE) {
        return false;
    }

    if(data[0] != START_FLAG_BYTE || data[data.size() - 1] != END_FLAG_BYTE) {
        return false;
    }

    this->startFlag = data[0];
    this->total = data[1];
    this->sequence = data[2];

    size_t dataEndPos;

    if (data.size() > 20) {
        dataEndPos = data.size() - 1 - 2; // fcs битов 9 чтобы 2^7 (от 0 до 7) было больше чем 16 * 8 + 1 плюс 1 бит secded
    }
    else {
        dataEndPos = data.size() - 1 - 1;
    }

    this->data.assign(data.begin() + HEADER_SIZE, data.begin() + dataEndPos);

    this->fcs.assign(data.begin() + dataEndPos, data.end() - 1);

    this->endFlag = data[data.size() - 1];
    return true;
}

int Frame::correctErrors() {
    return HammingEncoder::correctErrors(this->data, this->fcs);
}

int Frame::simulateErrors() {
    return ErrorSimulator::simulateErrors(this->data);
}
