#ifndef MAINCONTROLLER_HPP
#define MAINCONTROLLER_HPP
#include <engine/core/Controller.hpp>
#include <engine/graphics/BloomEffect.hpp>
#include <glm/glm.hpp>

namespace app {
class MainController : public engine::core::Controller {
    void initialize() override;

    bool loop() override;

    void draw_temple();

    void update() override;

    void update_camera();

    void update_action();

    void begin_draw() override;

    void draw_petal();

    void draw_ground();

    void draw_tree();

    void draw() override;

    void draw_skybox();

    void draw_lamp();

    void end_draw() override;

    //za eventove
    std::vector<glm::mat4> m_petal_matrices;
    std::vector<glm::mat4> m_original_petal_matrices;
    bool m_initialized = false;

    //bloom
    engine::graphics::BloomEffect m_bloom;

public:
    std::string_view name() const override { return "app::MainController"; }

    void on_window_resize(int width, int height);

    //za fejd svetla u event b
    glm::vec3 current_ambient = glm::vec3(0.2f);
    glm::vec3 current_diffuse = glm::vec3(0.5f);
    glm::vec3 current_specular = glm::vec3(0.8f);

    glm::vec3 target_ambient = current_ambient;
    glm::vec3 target_diffuse = current_diffuse;
    glm::vec3 target_specular = current_specular;
};
}

#endif //MAINCONTROLLER_HPP