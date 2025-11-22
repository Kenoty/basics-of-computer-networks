#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "HammingEncoder.h"

#define START_FLAG_BYTE 0x0B
#define HEADER_SIZE 3
#define TRAILER_SIZE 1
#define END_FLAG_BYTE 0x0C
#define ESCAPE_BYTE 0x7D
#define XOR_MASK 0x50

class Frame {
public:
    Frame(): startFlag(0), total(0), sequence(0), data(0), fcs(0), endFlag(0) {};

    Frame(uint8_t sequence, uint8_t total, const std::vector<uint8_t>& data)
        : startFlag(START_FLAG_BYTE), sequence(sequence), total(total), data(data), endFlag(END_FLAG_BYTE) {
        fcs = HammingEncoder::calculateControlBits(this->data);
    }

    Frame(uint8_t sequence, uint8_t total, const std::string& data)
        : startFlag(START_FLAG_BYTE), sequence(sequence), total(total), endFlag(END_FLAG_BYTE) {
        setData(data);
        fcs = HammingEncoder::calculateControlBits(this->data);
    }

    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);

    uint8_t getStartFlag() const { return startFlag; }
    uint8_t getTotal() const { return total; }
    uint8_t getSequence() const { return sequence; }
    const std::vector<uint8_t>& getData() const { return data; }
    const std::vector<uint8_t>& getFcs() const { return fcs; }
    uint8_t getEndFlag() const { return endFlag; }

    void setStartFlag(uint8_t flag) { this->startFlag = flag; }
    void setTotal(uint8_t total) { this->total = total; }
    void setSequence(uint8_t sequence) { this->sequence = sequence; }
    void setData(const std::vector<uint8_t>& data);
    void setData(const std::string& data);
    void setFcs(std::vector<uint8_t> fcs) { this->fcs = fcs; }
    void setEndFlag(uint8_t flag) { this->endFlag = flag; }

    std::string dataToString() const;

    int correctErrors();
    int simulateErrors();

private:
    uint8_t startFlag;
    uint8_t total;
    uint8_t sequence;
    std::vector<uint8_t> data;
    std::vector<uint8_t> fcs;
    uint8_t endFlag;
};
