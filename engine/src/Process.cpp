#include "engine/core/Process.hpp"

namespace engine::core {

Process::~Process() = default;

bool Process::initialize() { return true; }

void Process::finalize() {}

Process::State Process::state() const { return m_state; }

void Process::setState(State newState) { m_state = newState; }

void Process::setDuration(float duration) { m_duration = duration; }

float Process::duration() const { return m_duration; }

}
