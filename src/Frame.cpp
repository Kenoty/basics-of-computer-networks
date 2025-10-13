#include "Frame.h"

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

    bytes.push_back(this->endFlag);

    return bytes;
}

bool Frame::deserialize(const std::vector<uint8_t>& data) {

    if(data.size() < HEADER_SIZE + 2) {
        return false;
    }

    if(data[0] != FLAG_BYTE || data[data.size() - 1] != FLAG_BYTE) {
        return false;
    }


    this->startFlag = data[0];
    this->total = data[1];
    this->sequence = data[2];

    this->data.assign(data.begin() + HEADER_SIZE, data.end() - 1);

    this->endFlag = data[data.size() - 1];
    return true;
}
