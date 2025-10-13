#pragma once

#include "Frame.h"
#include <vector>

#define ESCAPE_BYTE 0x7D
#define XOR_MASK 0x20

class FrameManager {
public:
    FrameManager() {};

    std::vector<Frame> packMessage(const std::string& message);
    std::string unpackMessage(const Frame& frame);
    std::vector<std::string> byteStuff(const std::vector<Frame>& frames);
    Frame byteUnstuff(const std::string& bytes);

    static bool isValidFrame(const std::vector<uint8_t>& data);
    static bool isValidFrame(const std::string& data);
};
