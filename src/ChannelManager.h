#pragma once
#include <random>
#include <atomic>
#include <chrono>

// CSMA/CD константы
#define SLOT_TIME_MS 1  // 512 бит при 10 Мбит/с = 51.2 мс
#define MAX_ATTEMPTS 16    // Максимальное число попыток
#define JAM_SIGNAL_SIZE 32 // 32 бита jam-сигнала

class ChannelManager {
public:
    static ChannelManager& getInstance();

    void setEmulationEnabled(bool enabled) { m_emulationEnabled = enabled; }

    bool isChannelBusy();
    bool isCollisionOccurred();

    int calculateBackoffDelay(int attempt);
    void setJamSignal(bool jam) { m_jamSignal = jam; }
    bool getJamSignal() const { return m_jamSignal; }
    double getSlotTime() const { return SLOT_TIME_MS; }
    int getMaxAttempts() const { return MAX_ATTEMPTS; }

    void incrementCollisions() { m_collisionCount++; }
    int getCollisionCount() const { return m_collisionCount; }
    void resetStatistics() { m_collisionCount = 0; }

private:
    ChannelManager();

    std::mt19937 m_generator;
    std::uniform_real_distribution<double> m_probDist;
    std::atomic<bool> m_jamSignal{false};
    std::atomic<int> m_collisionCount{0};
    bool m_emulationEnabled = false;
};
