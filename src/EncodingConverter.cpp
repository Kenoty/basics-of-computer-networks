#include "EncodingConverter.h"

std::string EncodingConverter::utf8ToWindows1251(const std::string& utf8) {
    if (utf8.empty()) return "";

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.length(), NULL, 0);
    if (wideLen == 0) return "";

    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.length(), &wideStr[0], wideLen);

    int cp1251Len = WideCharToMultiByte(1251, 0, wideStr.c_str(), wideLen, NULL, 0, NULL, NULL);
    if (cp1251Len == 0) return "";

    std::string cp1251Str(cp1251Len, 0);
    WideCharToMultiByte(1251, 0, wideStr.c_str(), wideLen, &cp1251Str[0], cp1251Len, NULL, NULL);

    return cp1251Str;
}

std::string EncodingConverter::windows1251ToUtf8(const std::string& cp1251) {
    if (cp1251.empty()) return "";

    int wideLen = MultiByteToWideChar(1251, 0, cp1251.c_str(), cp1251.length(), NULL, 0);
    if (wideLen == 0) return "";

    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(1251, 0, cp1251.c_str(), cp1251.length(), &wideStr[0], wideLen);

    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLen, NULL, 0, NULL, NULL);
    if (utf8Len == 0) return "";

    std::string utf8Str(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), wideLen, &utf8Str[0], utf8Len, NULL, NULL);

    return utf8Str;
}

bool EncodingConverter::isWindows1251Available() {
    return GetACP() == 1251 || true;
}
