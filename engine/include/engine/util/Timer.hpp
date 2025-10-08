#ifndef ENGINE_CORE_TIMER_HPP
#define ENGINE_CORE_TIMER_HPP

#include <chrono>

namespace engine::util {
    class Timer {
    public:
        void start() {
            m_start = std::chrono::high_resolution_clock::now();
            m_end   = {};
        }

        void stop() {
            m_end = std::chrono::high_resolution_clock::now();
        }

        float elapsed() const {
            using clock   = std::chrono::high_resolution_clock;
            auto end_time = (m_end.time_since_epoch().count() == 0)
                                ? clock::now()
                                : m_end;
            std::chrono::duration<float> diff = end_time - m_start;
            return diff.count();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start{};
        std::chrono::time_point<std::chrono::high_resolution_clock> m_end{};
    };
}

#endif
