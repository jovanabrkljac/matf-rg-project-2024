#include <MovingPetals.hpp>
#include <engine/platform/PlatformController.hpp>
#include <engine/resources/ResourcesController.hpp>
#include <engine/core/Controller.hpp>
#include <MainController.hpp>
#include <spdlog/spdlog.h>

namespace app {

MovingPetals::MovingPetals(std::vector<glm::mat4> &petals,
                           std::vector<glm::mat4> &originals,
                           float duration)
: m_petals(petals)
, m_originals(originals) { set_duration(duration); }

bool MovingPetals::initialize() {
    spdlog::info("MovingPetals process started (duration = {}s)", m_duration);
    m_timer.start();
    m_elapsed = 0.0f;
    set_state(State::Running);
    return true;
}

void MovingPetals::update() {
    if (m_state != State::Running) return;

    m_elapsed = m_timer.elapsed();

    animate_petals(m_elapsed);
    update_lighting(m_elapsed);

    if (m_elapsed >= m_duration) {
        set_state(State::Done);
        spdlog::info("MovingPetals finished after {:.2f}s", m_elapsed);
    }
}

void MovingPetals::animate_petals(float elapsed) {
    for (size_t i = 0; i < m_petals.size(); ++i) {
        float angle = glm::radians(elapsed * 20.0f + static_cast<float>(i));
        float yOffset = sin(elapsed * 1.2f + static_cast<float>(i)) * 0.4f;
        glm::mat4 offset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, yOffset, 0.0f));
        offset = glm::rotate(offset, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        m_petals[i] = offset * m_originals[i];
    }

    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto petalModel = resource->model("petal");
    petalModel->set_instance_data(m_petals);
}

void MovingPetals::update_lighting(float elapsed) {
    auto mainCtrl = engine::core::Controller::get<app::MainController>();
    if (!mainCtrl) return;

    glm::vec3 normalAmbient(0.2f);
    glm::vec3 normalDiffuse(0.5f);
    glm::vec3 normalSpecular(0.8f);

    glm::vec3 brightAmbient(0.7f, 0.35f, 0.35f);
    glm::vec3 brightDiffuse(0.9f, 0.7f, 0.7f);
    glm::vec3 brightSpecular(1.0f, 0.8f, 0.8f);

    if (elapsed >= 4.0f && elapsed < 6.0f) {
        float t = (elapsed - 4.0f) / 2.0f;
        mainCtrl->current_ambient = glm::mix(normalAmbient, brightAmbient, t);
        mainCtrl->current_diffuse = glm::mix(normalDiffuse, brightDiffuse, t);
        mainCtrl->current_specular = glm::mix(normalSpecular, brightSpecular, t);
    } else if (elapsed >= 6.0f && elapsed < 10.0f) {
        mainCtrl->current_ambient = brightAmbient;
        mainCtrl->current_diffuse = brightDiffuse;
        mainCtrl->current_specular = brightSpecular;
    } else if (elapsed >= 10.0f && elapsed < 12.0f) {
        float t = (elapsed - 10.0f) / 2.0f;
        mainCtrl->current_ambient = glm::mix(brightAmbient, normalAmbient, t);
        mainCtrl->current_diffuse = glm::mix(brightDiffuse, normalDiffuse, t);
        mainCtrl->current_specular = glm::mix(brightSpecular, normalSpecular, t);
    } else if (elapsed >= 12.0f) {
        mainCtrl->current_ambient = normalAmbient;
        mainCtrl->current_diffuse = normalDiffuse;
        mainCtrl->current_specular = normalSpecular;
    }
}

void MovingPetals::finalize() {
    m_timer.stop();
    spdlog::info("MovingPetals process finalized - resetting petals.");
    m_petals = m_originals;

    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto petalModel = resource->model("petal");
    petalModel->set_instance_data(m_petals);

    auto mainCtrl = engine::core::Controller::get<app::MainController>();
    if (mainCtrl) {
        mainCtrl->current_ambient = glm::vec3(0.2f);
        mainCtrl->current_diffuse = glm::vec3(0.5f);
        mainCtrl->current_specular = glm::vec3(0.8f);
    }
}

}
