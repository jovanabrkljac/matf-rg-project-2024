#ifndef PROCESS_HPP
#define PROCESS_HPP

namespace engine::core {
    class Process {
    public:
        enum class State { JustCreated, Running, Paused, Done };

        virtual ~Process();

        virtual bool initialize();

        virtual void update() = 0;

        virtual void finalize();

        State state() const;

        void set_state(State new_state);

        void set_duration(float duration);

        float duration() const;

    protected:
        State m_state    = State::JustCreated;
        float m_duration = 0.0f;
    };
}
#endif //PROCESS_HPP
