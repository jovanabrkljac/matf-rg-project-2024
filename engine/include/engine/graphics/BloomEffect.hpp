//
// Created by Jovana on 16.7.2025..
//

#ifndef BLOOMEFFECT_HPP
#define BLOOMEFFECT_HPP
#include <engine/resources/Shader.hpp>
#include <glm/glm.hpp>

namespace engine::graphics {
    class BloomEffect {
    public:
        BloomEffect();

        ~BloomEffect();

        void initialize(int width, int height, resources::Shader *blur_shader, resources::Shader *final_shader);

        void resize(int width, int height);

        void begin();

        void end(const glm::vec2 &viewport_size);

    private:
        int m_width{0}, m_height{0};
        unsigned int m_scene_fbo{0};
        unsigned int m_scene_color_tex{0};
        unsigned int m_bright_tex{0};
        unsigned int m_depth_rbo{0};
        unsigned int m_pingpong_fbo[2]{0, 0};
        unsigned int m_pingpong_tex[2]{0, 0};

        resources::Shader *m_blur_shader  = nullptr;
        resources::Shader *m_final_shader = nullptr;

        unsigned int m_quad_vao{0};
        unsigned int m_quad_vbo{0};

        void draw_quad();
    };
} // namespace engine::graphics
#endif //BLOOMEFFECT_HPP
