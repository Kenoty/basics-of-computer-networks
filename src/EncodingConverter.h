#pragma once
#include <string>
#include <windows.h>

class EncodingConverter {
public:
    static std::string utf8ToWindows1251(const std::string& utf8);
    static std::string windows1251ToUtf8(const std::string& cp1251);
    static bool isWindows1251Available();
};
