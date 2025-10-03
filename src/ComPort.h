#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

class ComPort {
public:
    using DataReceivedCallback = std::function<void(const std::string& data)>;

    ComPort();
    ~ComPort();

    bool open(const std::string& portName, int baudRate = 9600);
    void close();
    bool isOpen() const { return m_isOpen; };

    bool writeData(const std::string& data);
    bool writeData(const char* data, size_t length);

    bool startAsyncReading(const DataReceivedCallback& callback = nullptr);
    void stopAsyncReading();

    bool setBaudRate(int baudRate);
    bool setTimeout(int readIntervalMs = 50, int readTotalMs = 50, int writeTotalMs = 50);

    static std::vector<std::string> getAvailablePorts();
    static bool portExists(const std::string& portName);

    std::string getPortName() const { return m_portName; };
    int getBaudRate() const { return m_baudRate; };
private:
    bool configurePort();
    void readingThreadFunc();

    HANDLE m_hPort = INVALID_HANDLE_VALUE;
    std::string m_portName;
    int m_baudRate = 9600;

    std::atomic<bool> m_isOpen{false};
    std::atomic<bool> m_keepReading{false};
    std::thread m_readingThread;

    DataReceivedCallback m_dataCallback;
};
