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
, m_originals(originals) { set_duration(duration); }

bool MovingPetals::initialize() {
    spdlog::info("MovingPetals process started (duration = {}s)", m_duration);
    m_timer.start();
    m_elapsed = 0.0f;
    m_light_changed = false;
    m_light_restored = false;
    set_state(State::Running);
    return true;
}

void MovingPetals::update(float dt) {
    if (m_state != State::Running) return;

    if (dt > 1.0f) dt *= 0.001f;

    m_elapsed += dt;
    animate_petals(dt);
    update_lighting(dt);

    if (m_elapsed >= m_duration) {
        finalize();
        set_state(State::Done);
        spdlog::info("MovingPetals finished after {:.2f}s", m_elapsed);
    }
}

// animacija latica
void MovingPetals::animate_petals(float dt) {
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
void MovingPetals::update_lighting(float dt) {
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
        mainCtrl->current_ambient = glm::mix(normalAmbient, brightAmbient, t);
        mainCtrl->current_diffuse = glm::mix(normalDiffuse, brightDiffuse, t);
        mainCtrl->current_specular = glm::mix(normalSpecular, brightSpecular, t);
    } else if (m_elapsed >= 6.0f && m_elapsed < 10.0f) {
        mainCtrl->current_ambient = brightAmbient;
        mainCtrl->current_diffuse = brightDiffuse;
        mainCtrl->current_specular = brightSpecular;
    } else if (m_elapsed >= 10.0f && m_elapsed < 12.0f) {
        float t = (m_elapsed - 10.0f) / 2.0f;
        mainCtrl->current_ambient = glm::mix(brightAmbient, normalAmbient, t);
        mainCtrl->current_diffuse = glm::mix(brightDiffuse, normalDiffuse, t);
        mainCtrl->current_specular = glm::mix(brightSpecular, normalSpecular, t);
    } else if (m_elapsed >= 12.0f) {
        mainCtrl->current_ambient = normalAmbient;
        mainCtrl->current_diffuse = normalDiffuse;
        mainCtrl->current_specular = normalSpecular;
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
        mainCtrl->current_ambient = glm::vec3(0.2f);
        mainCtrl->current_diffuse = glm::vec3(0.5f);
        mainCtrl->current_specular = glm::vec3(0.8f);
    }
}

}
