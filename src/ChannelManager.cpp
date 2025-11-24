#include "ChannelManager.h"

ChannelManager& ChannelManager::getInstance() {
    static ChannelManager instance;
    return instance;
}

ChannelManager::ChannelManager()
    : m_generator(std::random_device{}())
    , m_probDist(0.0, 1.0) {
}

bool ChannelManager::isChannelBusy() {
    if (!m_emulationEnabled) {
        return false;
    }
    return m_probDist(m_generator) < 0.75;
}

bool ChannelManager::isCollisionOccurred() {
    if (!m_emulationEnabled) {
        return false;
    }
    return m_probDist(m_generator) < 0.25;
}

int ChannelManager::calculateBackoffDelay(int attempt) {
    // Стандартная формула: случайное число от 0 до 2^k - 1
    int k = std::min(attempt, 10);
    int maxDelay = (1 << k) - 1;
    std::uniform_int_distribution<int> delayDist(0, maxDelay);
    return delayDist(m_generator);
}
