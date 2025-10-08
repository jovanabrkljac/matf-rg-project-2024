#ifndef MOVINGPETALS_HPP
#define MOVINGPETALS_HPP

#include <engine/core/Process.hpp>
#include <engine/util/Timer.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace app {

class MovingPetals : public engine::core::Process {
public:
    MovingPetals(std::vector<glm::mat4> &petals,
                 std::vector<glm::mat4> &originals,
                 float duration);

    bool initialize() override;

    void update() override;

    void finalize() override;

private:
    std::vector<glm::mat4> &m_petals;
    std::vector<glm::mat4> &m_originals;

    engine::util::Timer m_timer;
    float m_elapsed{0.0f};

    void animate_petals(float elapsed);

    void update_lighting(float elapsed);
};

}

#endif //MOVINGPETALS_HPP
