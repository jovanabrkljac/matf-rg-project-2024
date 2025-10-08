//
// Created by Jovana on 14.7.2025..
//

#ifndef GUICONTROLLER_HPP
#define GUICONTROLLER_HPP

#include <engine/core/Controller.hpp>
#include <glm/glm.hpp>

namespace app {

class GUIController : public engine::core::Controller {
public:
    float get_point_light_intensity() const { return m_point_light_intensity; }
    float get_dir_light_intensity() const { return m_dir_light_intensity; }
    glm::vec3 get_point_light_color() const { return m_point_light_color; }

    std::string_view name() const override { return "app::GUIController"; }

private:
    float m_point_light_intensity = 2.0f;
    float m_dir_light_intensity = 1.5f;
    glm::vec3 m_point_light_color = glm::vec3(1.0f, 0.21f, 0.21f);

    void initialize() override;

    void draw() override;

    void poll_events() override;
};

}// namespace app

#endif // GUICONTROLLER_HPP
