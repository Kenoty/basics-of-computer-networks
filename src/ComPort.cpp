#include "ComPort.h"
#include "EncodingConverter.h"
#include <iostream>

ComPort::ComPort() {};

ComPort::~ComPort() {
    close();
}

bool ComPort::open(const std::string& portName, int baudRate) {
    if (m_isOpen) {
        close();
    }

    m_portName = portName;
    m_baudRate = baudRate;

    m_hPort = CreateFileA(
        ("\\\\.\\" + portName).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (m_hPort == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия порта " << portName << std::endl;
        return false;
    }

    if (!configurePort()) {
        std::cerr << "Ошибка конфигурации порта " << portName << std::endl;
        CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
        return false;
    }

    m_isOpen = true;
    return true;
}

void ComPort::close() {
    stopAsyncReading();

    if (m_hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
    }

    m_isOpen = false;
}

bool ComPort::configurePort() {
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(m_hPort, &dcb)) {
        return false;
    }

    dcb.BaudRate = m_baudRate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(m_hPort, &dcb)) {
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    return SetCommTimeouts(m_hPort, &timeouts);
}

bool ComPort::writeData(const std::string& data) {
    if (!isOpen()) return false;

    std::string encodedData = EncodingConverter::utf8ToWindows1251(data);
    if (encodedData.empty() && !data.empty()) {
        std::cerr << "Ошибка кодировки при конвертации в Windows-1251" << std::endl;
        return false;
    }

    return writeData(encodedData.c_str(), encodedData.length());
}

bool ComPort::writeData(const char* data, size_t length) {
    if (!isOpen()) return false;

    DWORD bytesWritten;
    BOOL success = WriteFile(m_hPort, data, length, &bytesWritten, NULL);

    return success && (bytesWritten == length);
}

bool ComPort::startAsyncReading(const DataReceivedCallback& callback) {
    if (!m_isOpen || m_keepReading) {
        return false;
    }

    m_dataCallback = callback;
    m_keepReading = true;
    m_readingThread = std::thread(&ComPort::readingThreadFunc, this);

    return true;
}

void ComPort::stopAsyncReading() {
    m_keepReading = false;
    if (m_readingThread.joinable()) {
        m_readingThread.join();
    }
    m_dataCallback = nullptr;
}

void ComPort::readingThreadFunc() {
    char buffer[1024];
    DWORD bytesRead;

    while (m_keepReading) {
        if (ReadFile(m_hPort, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0'; //последнее что добавил, но не собрал
                std::string receivedData(buffer, bytesRead);

                std::string utf8Data = EncodingConverter::windows1251ToUtf8(receivedData);

                if (!utf8Data.empty() && m_dataCallback) {
                    m_dataCallback(utf8Data);
                } else if (m_dataCallback) {
                    m_dataCallback(receivedData);
                }
            }
        } else {
            // Ошибка чтения (кроме случая, когда мы сами останавливаем чтение)
            if (m_keepReading) {
                DWORD error = GetLastError();
                // Можно обработать специфические ошибки
                break;
            }
        }
    }
}

bool ComPort::setBaudRate(int baudRate) {
    if (!isOpen()) return false;

    m_baudRate = baudRate;
    return configurePort();
}

bool ComPort::setTimeout(int readIntervalMs, int readTotalMs, int writeTotalMs) {
    if (!m_isOpen) return false;

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = readIntervalMs;
    timeouts.ReadTotalTimeoutConstant = readTotalMs;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = writeTotalMs;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    return SetCommTimeouts(m_hPort, &timeouts);
}

std::vector<std::string> ComPort::getAvailablePorts() {
    std::vector<std::string> ports;

    for (int i = 1; i <= 256; ++i) {
        std::string portName = "COM" + std::to_string(i);
        HANDLE hPort = CreateFileA(("\\\\.\\" + portName).c_str(),
                                   GENERIC_READ | GENERIC_WRITE,
                                   0, NULL, OPEN_EXISTING, 0, NULL);

        if (hPort != INVALID_HANDLE_VALUE) {
            ports.push_back(portName);
            CloseHandle(hPort);
        }
    }

    return ports;
}

bool ComPort::portExists(const std::string& portName) {
    HANDLE hPort = CreateFileA(("\\\\.\\" + portName).c_str(),
                               GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(hPort);
        return true;
    }

    return false;
}
