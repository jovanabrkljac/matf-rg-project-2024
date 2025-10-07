#include <MovingPetals.hpp>
#include <engine/platform/PlatformController.hpp>
#include <engine/resources/ResourcesController.hpp>
#include <engine/core/Controller.hpp>
#include <MainController.hpp>
#include <spdlog/spdlog.h>

namespace app {
// proces koji animira latice i menja svetlo tokom vremena
MovingPetals::MovingPetals(std::vector<glm::mat4> &petals,
                           std::vector<glm::mat4> &originals,
                           float duration)
: m_petals(petals)
, m_originals(originals) { setDuration(duration); }

bool MovingPetals::initialize() {
    spdlog::info("MovingPetals process started (duration = {}s)", m_duration);
    m_timer.start();
    m_elapsed = 0.0f;
    m_lightChanged = false;
    m_lightRestored = false;
    setState(State::Running);
    return true;
}

void MovingPetals::update(float dt) {
    if (m_state != State::Running) return;

    if (dt > 1.0f) dt *= 0.001f;

    m_elapsed += dt;
    animatePetals(dt);
    updateLighting(dt);

    if (m_elapsed >= m_duration) {
        finalize();
        setState(State::Done);
        spdlog::info("MovingPetals finished after {:.2f}s", m_elapsed);
    }
}

// animacija latica
void MovingPetals::animatePetals(float dt) {
    for (size_t i = 0; i < m_petals.size(); ++i) {
        float angle = glm::radians(m_elapsed * 20.0f + static_cast<float>(i));
        float yOffset = sin(m_elapsed * 1.2f + static_cast<float>(i)) * 0.4f;
        glm::mat4 offset = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, yOffset, 0.0f));
        offset = glm::rotate(offset, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        m_petals[i] = offset * m_originals[i];
    }

    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto petalModel = resource->model("petal");
    petalModel->set_instance_data(m_petals);
}

// pojacava svetlo roze boje i smanjuje ga
void MovingPetals::updateLighting(float dt) {
    auto mainCtrl = engine::core::Controller::get<app::MainController>();
    if (!mainCtrl) return;

    glm::vec3 normalAmbient = glm::vec3(0.2f);
    glm::vec3 normalDiffuse = glm::vec3(0.5f);
    glm::vec3 normalSpecular = glm::vec3(0.8f);

    glm::vec3 brightAmbient = glm::vec3(0.7f, 0.35f, 0.35f);
    glm::vec3 brightDiffuse = glm::vec3(0.9f, 0.7f, 0.7f);
    glm::vec3 brightSpecular = glm::vec3(1.0f, 0.8f, 0.8f);
    // ideja je da izmedju 4-6 sekunde svetlo se pojacava, 6-10s odrzava intenzitete, a posle 10s postepeno slabi
    if (m_elapsed >= 4.0f && m_elapsed < 6.0f) {
        float t = (m_elapsed - 4.0f) / 2.0f;
        mainCtrl->currentAmbient = glm::mix(normalAmbient, brightAmbient, t);
        mainCtrl->currentDiffuse = glm::mix(normalDiffuse, brightDiffuse, t);
        mainCtrl->currentSpecular = glm::mix(normalSpecular, brightSpecular, t);
    } else if (m_elapsed >= 6.0f && m_elapsed < 10.0f) {
        mainCtrl->currentAmbient = brightAmbient;
        mainCtrl->currentDiffuse = brightDiffuse;
        mainCtrl->currentSpecular = brightSpecular;
    } else if (m_elapsed >= 10.0f && m_elapsed < 12.0f) {
        float t = (m_elapsed - 10.0f) / 2.0f;
        mainCtrl->currentAmbient = glm::mix(brightAmbient, normalAmbient, t);
        mainCtrl->currentDiffuse = glm::mix(brightDiffuse, normalDiffuse, t);
        mainCtrl->currentSpecular = glm::mix(brightSpecular, normalSpecular, t);
    } else if (m_elapsed >= 12.0f) {
        mainCtrl->currentAmbient = normalAmbient;
        mainCtrl->currentDiffuse = normalDiffuse;
        mainCtrl->currentSpecular = normalSpecular;
    }
}

// zavrsava se proces i vraca se na pocetno stanje latica i svetla
void MovingPetals::finalize() {
    spdlog::info("MovingPetals process finalized - resetting petals.");
    m_petals = m_originals;

    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto petalModel = resource->model("petal");
    petalModel->set_instance_data(m_petals);

    auto mainCtrl = engine::core::Controller::get<app::MainController>();
    if (mainCtrl) {
        mainCtrl->currentAmbient = glm::vec3(0.2f);
        mainCtrl->currentDiffuse = glm::vec3(0.5f);
        mainCtrl->currentSpecular = glm::vec3(0.8f);
    }
}

}
