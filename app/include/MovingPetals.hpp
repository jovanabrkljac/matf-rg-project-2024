#ifndef MOVINGPETALS_HPP
#define MOVINGPETALS_HPP

#include <engine/core/Process.hpp>
#include <engine/core/Timer.hpp>
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

    void update(float dt) override;

    void finalize() override;

private:
    std::vector<glm::mat4> &m_petals;
    std::vector<glm::mat4> &m_originals;

    engine::core::Timer m_timer;
    float m_elapsed{0.0f};
    bool m_lightChanged{false};
    bool m_lightRestored{false};

    glm::vec3 m_startAmbient{0.2f};
    glm::vec3 m_startDiffuse{0.5f};
    glm::vec3 m_startSpecular{0.8f};

    glm::vec3 m_targetAmbient{0.6f, 0.28f, 0.30f};
    glm::vec3 m_targetDiffuse{0.75f, 0.60f, 0.65f};
    glm::vec3 m_targetSpecular{0.9f, 0.7f, 0.75f};

    void animatePetals(float dt);

    void updateLighting(float dt);
};

}

#endif //MOVINGPETALS_HPP
