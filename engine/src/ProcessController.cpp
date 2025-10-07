#include "engine/core/ProcessController.hpp"
#include <engine/platform/PlatformController.hpp>
#include <spdlog/spdlog.h>

namespace engine::core {

void ProcessController::add(std::unique_ptr<Process> process) { m_processes.push_back(std::move(process)); }

void ProcessController::update() {
    auto platform = engine::platform::PlatformController::get<engine::platform::PlatformController>();
    float dt = platform->dt();

    // ako platform->dt() vraca milisekunde, pretvori u sekunde
    if (dt > 1.0f) dt *= 0.001f;

    for (auto it = m_processes.begin(); it != m_processes.end();) {
        auto &process = *it;
        // inizijalizacija procesa ako je tek kreiran
        if (process->state() == Process::State::JustCreated) {
            if (process->initialize()) process->setState(Process::State::Running);
            else process->setState(Process::State::Done);
        }
        // azuriraj aktivne procese
        if (process->state() == Process::State::Running) process->update(dt);
        // ukloni zavrsene procese
        if (process->state() == Process::State::Done) {
            process->finalize();
            it = m_processes.erase(it);
        } else { ++it; }
    }
}

}
