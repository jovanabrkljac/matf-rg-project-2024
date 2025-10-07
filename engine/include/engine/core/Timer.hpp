#ifndef ENGINE_CORE_TIMER_HPP
#define ENGINE_CORE_TIMER_HPP

#include <chrono>

namespace engine::core {

class Timer {
public:
    Timer() : m_running(false)
          , m_start() {}
    // pokrece merenje vremena
    void start() {
        m_start = std::chrono::high_resolution_clock::now();
        m_running = true;
    }
    // zaustavlja tajmer
    void stop() { m_running = false; }
    // resetovanje tajmera
    void reset() { start(); }
    // vraca proteklo vreme u sekundama
    float elapsed() const {
        if (!m_running) return 0.0f;
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> diff = now - m_start;
        return diff.count();
    }
    // proverava da li tajmer trenutno radi
    bool running() const { return m_running; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    bool m_running;
};

}

#endif
