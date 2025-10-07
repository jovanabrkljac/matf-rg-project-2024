
#ifndef PROCESSCONTROLLER_HPP
#define PROCESSCONTROLLER_HPP

#include "engine/core/Process.hpp"
#include "engine/core/Controller.hpp"
#include <vector>
#include <memory>

namespace engine::core {

class ProcessController : public Controller {
public:
    void add(std::unique_ptr<Process> process);

    void update() override;

private:
    std::vector<std::unique_ptr<Process> > m_processes;
};

}

#endif //PROCESSCONTROLLER_HPP
