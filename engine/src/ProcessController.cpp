#include "engine/core/ProcessController.hpp"
#include <engine/platform/PlatformController.hpp>
#include <spdlog/spdlog.h>

namespace engine::core {

void ProcessController::add(std::unique_ptr<Process> process) { m_processes.push_back(std::move(process)); }

void ProcessController::update() {
    auto platform = engine::platform::PlatformController::get<engine::platform::PlatformController>();

    for (auto it = m_processes.begin(); it != m_processes.end();) {
        auto &process = *it;

        if (process->state() == Process::State::JustCreated) {
            if (process->initialize()) process->set_state(Process::State::Running);
            else process->set_state(Process::State::Done);
        }

        if (process->state() == Process::State::Running) process->update();

        if (process->state() == Process::State::Done) {
            process->finalize();
            it = m_processes.erase(it);
        } else { ++it; }
    }
}

}
