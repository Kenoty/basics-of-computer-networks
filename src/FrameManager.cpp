#include "FrameManager.h"
#include "EncodingConverter.h"
#include <iostream>

std::vector<Frame> FrameManager::packMessage(const std::string& message) {
    std::vector<Frame> result;
    int frameNum = 0;

    std::string encodedMessage = EncodingConverter::utf8ToWindows1251(message);
    if (encodedMessage.empty() && !message.empty()) {
        std::cerr << "Ошибка кодировки при конвертации в Windows-1251" << std::endl;
        return {};
    }

    if(encodedMessage.length() % 64 == 0) {
        frameNum = encodedMessage.length() / 64;
    }
    else {
        frameNum = encodedMessage.length() / 64 + 1;
    }

    for(int i = 0; i < frameNum; i++) {
        result.push_back(Frame(i + 1, frameNum, encodedMessage.substr(i * 64, 64)));
    }

    return result;
}

std::string FrameManager::unpackMessage(const Frame& frame) {
    std::string receivedData = frame.dataToString();

    std::string utf8Data = EncodingConverter::windows1251ToUtf8(receivedData);

    if (!utf8Data.empty()) {
        return utf8Data;
    }
    else {
        return std::string {};
    }
}

std::vector<std::string> FrameManager::byteStuff(const std::vector<Frame>& frames) {
    std::vector<std::string> result;

    for(int i = 0; i < frames.size(); i++) {
        std::vector<uint8_t> frameInBytes = frames[i].serialize();

        std::vector<uint8_t> stuffedFrame;

        stuffedFrame.push_back(frameInBytes.front());
        for(int j = 1; j < frameInBytes.size() - 1; j++) {
            if(frameInBytes[j] == START_FLAG_BYTE) {
                stuffedFrame.push_back(ESCAPE_BYTE);
                stuffedFrame.push_back(START_FLAG_BYTE ^ XOR_MASK);
            }
            else if (frameInBytes[j] == ESCAPE_BYTE) {
                stuffedFrame.push_back(ESCAPE_BYTE);
                stuffedFrame.push_back(ESCAPE_BYTE ^ XOR_MASK);
            }
            else {
                stuffedFrame.push_back(frameInBytes[j]);
            }
        }
        stuffedFrame.push_back(frameInBytes.back());

        result.push_back(std::string(stuffedFrame.begin(), stuffedFrame.end()));
    }

    return result;
}

Frame FrameManager::byteUnstuff(const std::string& bytes) {
    std::vector<uint8_t> stuffedBytes(bytes.begin(), bytes.end());

    std::vector<uint8_t> unstuffedBytes;

    unstuffedBytes.push_back(stuffedBytes.front());

    for(int i = 1; i < stuffedBytes.size() - 1; i++) {
        if(stuffedBytes[i] == ESCAPE_BYTE) {
            unstuffedBytes.push_back(stuffedBytes[++i] ^ XOR_MASK);
        }
        else {
            unstuffedBytes.push_back(stuffedBytes[i]);
        }
    }
    unstuffedBytes.push_back(stuffedBytes.back());

    Frame result = Frame();
    result.deserialize(unstuffedBytes);

    return result;
}

bool FrameManager::isValidFrame(const std::vector<uint8_t>& data) {
    if (data.size() < HEADER_SIZE + 2) {
        return false;
    }

    if (data.front() != START_FLAG_BYTE || data.back() != END_FLAG_BYTE) {
        return false;
    }

    return true;
}

bool FrameManager::isValidFrame(const std::string& data) {
    return isValidFrame(std::vector<uint8_t>(data.begin(), data.end()));
}
