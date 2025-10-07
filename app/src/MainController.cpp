#include <MainController.hpp>
#include <GuiController.hpp>
#include <MovingPetals.hpp>
#include <engine/platform/PlatformController.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <engine/graphics/OpenGL.hpp>
#include <engine/resources/ResourcesController.hpp>
#include <engine/resources/Shader.hpp>
#include <engine/core/ProcessController.hpp>
#include <spdlog/spdlog.h>
#include <random>

namespace app {

class MainPlatformEventObserver : public engine::platform::PlatformEventObserver {
    void on_mouse_move(engine::platform::MousePosition position) override;
    void on_scroll(engine::platform::MousePosition position) override;
    void on_window_resize(int width, int height) override;
};

void MainPlatformEventObserver::on_mouse_move(engine::platform::MousePosition position) {
    auto gui_controller = engine::core::Controller::get<GUIController>();
    if (!gui_controller->is_enabled()) {
        auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
        auto camera = graphics->camera();
        camera->rotate_camera(position.dx, position.dy);
    }
}

void MainPlatformEventObserver::on_scroll(engine::platform::MousePosition position) {
    auto gui_controller = engine::core::Controller::get<GUIController>();
    if (!gui_controller->is_enabled()) {
        auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
        auto platform = engine::platform::PlatformController::get<engine::platform::PlatformController>();

        auto camera = graphics->camera();
        auto mouse = platform->mouse();
        camera->zoom(mouse.scroll);
        graphics->perspective_params().FOV = glm::radians(camera->Zoom);
    }
}

void MainPlatformEventObserver::on_window_resize(int width, int height) {

    auto mainCtrl = engine::core::Controller::get<app::MainController>();
    if (mainCtrl) {
        mainCtrl->on_window_resize(width, height);
    }
}

void MainController::on_window_resize(int width, int height) {
    bloom.resize(width, height);
}

void MainController::initialize() {
    spdlog::info("MainController initialized !");
    auto platform = engine::platform::PlatformController::get<engine::platform::PlatformController>();
    platform->register_platform_event_observer(std::make_unique<MainPlatformEventObserver>());
    engine::graphics::OpenGL::enable_depth_testing();

    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto camera = graphics->camera();

    camera->Position = glm::vec3(6.0f, 5.0f, 28.0f);

    //bloom init
    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    engine::resources::Shader* blurShader  = resource->shader("bloom_blur");
    engine::resources::Shader* finalShader = resource->shader("bloom_final");
    bloom.initialize(platform->window()->width(), platform->window()->height(), blurShader, finalShader);

}

bool MainController::loop() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KeyId::KEY_ESCAPE).is_down()) return false;
    return true;
}

void MainController::update_camera() {
    auto gui_controller = engine::core::Controller::get<GUIController>();
    if (gui_controller->is_enabled()) return;

    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto camera = graphics->camera();
    float dt = platform->dt();

    if (platform->key(engine::platform::KeyId::KEY_W).is_down()) camera->move_camera(engine::graphics::Camera::Movement::FORWARD, dt);
    if (platform->key(engine::platform::KeyId::KEY_S).is_down()) camera->move_camera(engine::graphics::Camera::Movement::BACKWARD, dt);
    if (platform->key(engine::platform::KeyId::KEY_A).is_down()) camera->move_camera(engine::graphics::Camera::Movement::LEFT, dt);
    if (platform->key(engine::platform::KeyId::KEY_D).is_down()) camera->move_camera(engine::graphics::Camera::Movement::RIGHT, dt);
}

void MainController::update_action() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    static bool wasPDown = false;
    bool isPDown = platform->key(engine::platform::KeyId::KEY_P).is_down();

    if (isPDown && !wasPDown) {
        auto processCtrl = engine::core::Controller::get<engine::core::ProcessController>();
        processCtrl->add(std::make_unique<app::MovingPetals>(petalMatrices, originalPetalMatrices, 12.0f));
    }

    wasPDown = isPDown;
}

void MainController::update() {
    update_camera();
    update_action();
}

void MainController::begin_draw() { bloom.begin(); }

void MainController::draw() {
    draw_temple();
    draw_lamp();
    draw_petal();
    draw_skybox();
    draw_ground();
    draw_tree();
}

void MainController::end_draw() {
    auto platform = engine::platform::PlatformController::get<engine::platform::PlatformController>();
    bloom.end({platform->window()->width(), platform->window()->height()});
    platform->swap_buffers();
}

//crtanje objekata

void MainController::draw_temple() {
    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto gui = engine::core::Controller::get<app::GUIController>();

    engine::resources::Model *temple = resource->model("temple");
    engine::resources::Shader *shader = resource->shader("basic");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(6.0f, 0.0f, -2.0f));
    model = glm::scale(model, glm::vec3(0.3f));
    shader->set_mat4("model", model);
    shader->set_float("shininess", 32.0f);

    glm::vec3 ambient = currentAmbient * gui->get_dir_light_intensity();
    glm::vec3 diffuse = currentDiffuse * gui->get_dir_light_intensity();
    glm::vec3 specular = currentSpecular * gui->get_dir_light_intensity();

    shader->set_vec3("dirLight.direction", glm::vec3(-0.3f, -1.0f, -0.3f));
    shader->set_vec3("dirLight.ambient", ambient);
    shader->set_vec3("dirLight.diffuse", diffuse);
    shader->set_vec3("dirLight.specular", specular);

    glm::vec3 color = gui->get_point_light_color();
    float pIntensity = gui->get_point_light_intensity();

    shader->set_vec3("pointLights[0].position", glm::vec3(-2.0f, 3.5f, -0.6f));
    shader->set_vec3("pointLights[1].position", glm::vec3(14.0f, 3.5f, -0.6f));

    for (int i = 0; i < 2; ++i) {
        shader->set_float("pointLights[" + std::to_string(i) + "].constant", 1.0f);
        shader->set_float("pointLights[" + std::to_string(i) + "].linear", 0.14f);
        shader->set_float("pointLights[" + std::to_string(i) + "].quadratic", 0.07f);
        shader->set_vec3("pointLights[" + std::to_string(i) + "].ambient", color * 0.2f * pIntensity);
        shader->set_vec3("pointLights[" + std::to_string(i) + "].diffuse", color * 0.8f * pIntensity);
        shader->set_vec3("pointLights[" + std::to_string(i) + "].specular", color * 1.0f * pIntensity);
    }

    shader->set_vec3("viewPos", graphics->camera()->Position);

    temple->draw(shader);
}

void MainController::draw_petal() {
    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto petalModel = resource->model("petal");
    auto petalShader = resource->shader("petal");

    if (!initialized) {
        const int numPetals = 1500;
        const float spread = 30.0f;
        petalMatrices.clear();
        originalPetalMatrices.clear();

        std::mt19937 rng{42};
        std::uniform_real_distribution<float> u01{0.0f, 1.0f};

        for (int i = 0; i < numPetals; ++i) {
            float x = (u01(rng) - 0.5f) * spread;
            float y =  u01(rng) * 10.0f + 2.0f;
            float z = (u01(rng) - 0.5f) * spread;

            float rotation   = u01(rng) * glm::two_pi<float>();
            float scale = 0.2f + u01(rng) * 0.3f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x + 6.0f, y, z)); // jer mi je hram pomeren od centra za 6 po x-osi
            model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(scale));

            petalMatrices.push_back(model);
            originalPetalMatrices.push_back(model);
        }

        petalModel->set_instance_data(petalMatrices);
        initialized = true;
    }


    petalShader->use();
    petalShader->set_mat4("projection", graphics->projection_matrix());
    petalShader->set_mat4("view", graphics->camera()->view_matrix());
    petalShader->set_int("texture_diffuse1", 0);
    petalModel->draw_instanced(petalShader, static_cast<int>(petalMatrices.size()));
}

void MainController::draw_lamp() {
    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();

    engine::resources::Model *lampModel = resource->model("lamp");
    engine::resources::Shader *lampShader = resource->shader("lamp");

    lampShader->use();
    lampShader->set_mat4("projection", graphics->projection_matrix());
    lampShader->set_mat4("view", graphics->camera()->view_matrix());

    glm::vec3 lampPositions[] = {glm::vec3(-2.0f, 0.69f, 0.0f), glm::vec3(14.0f, 0.69f, 0.0f)};
    auto gui = engine::core::Controller::get<app::GUIController>();
    glm::vec3 lightColor = gui->get_point_light_color();

    for (int i = 0; i < 2; ++i) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lampPositions[i]);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.7f));
        lampShader->set_mat4("model", model);
        glm::vec3 emissionColor = lightColor * 2.0f;
        lampShader->set_vec3("emissionColor", emissionColor);
        lampShader->set_float("bloomThreshold", 0.5f);
        lampModel->draw(lampShader);
    }
}

void MainController::draw_ground() {
    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();

    auto groundModel = resource->model("ground");
    auto shader = resource->shader("ground");

    shader->use();
    shader->set_vec3("tintColor", glm::vec3(0.15f, 0.45f, 0.15f)); // boja slicna krovu
    shader->set_float("tintStrength", 0.3f);

    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.6f, 0.0f));
    model=glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(50.0f));
    shader->set_mat4("model", model);
    groundModel->draw(shader);
}

void MainController::draw_tree() {
    auto resource = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();

    auto treeModel = resource->model("tree");
    auto shader = resource->shader("basic");

    auto deadTreeModel = resource->model("dead_tree");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(19.0f, 0.0f, -3.0f));
    model = glm::scale(model, glm::vec3(0.3f));
    shader->set_mat4("model", model);
    treeModel->draw(shader);

    glm::mat4 model2 = glm::mat4(1.0f);
    model2 = glm::translate(model2, glm::vec3(-7.0f, 0.0f, -3.0f));
    model2=glm::rotate(model2, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    //model2 = glm::scale(model2, glm::vec3(0.5f));
    shader->set_mat4("model", model2);
    deadTreeModel->draw(shader);
}

void MainController::draw_skybox() {
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("skybox");
    auto skybox = engine::core::Controller::get<engine::resources::ResourcesController>()->skybox("sky");
    engine::core::Controller::get<engine::graphics::GraphicsController>()->draw_skybox(shader, skybox);
}

}
