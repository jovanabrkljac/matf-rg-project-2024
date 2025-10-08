#include "engine/graphics/BloomEffect.hpp"
#include <engine/resources/Shader.hpp>
#include <glad/glad.h>
#include <engine/graphics/OpenGL.hpp>
#include <engine/util/Errors.hpp>

using namespace engine::graphics;

BloomEffect::BloomEffect() = default;

BloomEffect::~BloomEffect() = default;

void BloomEffect::terminate() {
    CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_scene_fbo);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_scene_color_tex);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_bright_tex);
    CHECKED_GL_CALL(glDeleteRenderbuffers, 1, &m_depth_rbo);
    for (int i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glDeleteTextures, 1, &m_pingpong_tex[i]);
    }
    if (m_quad_vao)
        CHECKED_GL_CALL(glDeleteVertexArrays, 1, &m_quad_vao);
    if (m_quad_vbo)
        CHECKED_GL_CALL(glDeleteBuffers, 1, &m_quad_vbo);
}

void BloomEffect::initialize(int w, int h, resources::Shader *blurShader, resources::Shader *finalShader) {
    m_width = w;
    m_height = h;
    m_blur_shader = blurShader;
    m_final_shader = finalShader;


    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_scene_fbo);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_scene_fbo);


    CHECKED_GL_CALL(glGenTextures, 1, &m_scene_color_tex);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_scene_color_tex);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_scene_color_tex, 0);

    // samo svetli pikseli
    CHECKED_GL_CALL(glGenTextures, 1, &m_bright_tex);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_bright_tex);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_bright_tex, 0);

    // rbo
    CHECKED_GL_CALL(glGenRenderbuffers, 1, &m_depth_rbo);
    CHECKED_GL_CALL(glBindRenderbuffer, GL_RENDERBUFFER, m_depth_rbo);
    CHECKED_GL_CALL(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    CHECKED_GL_CALL(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_rbo);


    GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    CHECKED_GL_CALL(glDrawBuffers, 2, bufs);

    RG_GUARANTEE(
            glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "Framebuffer not complete!"
            );
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    //ping-pong
    for (int i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glGenFramebuffers, 1, &m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glGenTextures, 1, &m_pingpong_tex[i]);
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_tex[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpong_tex[i], 0);
        RG_GUARANTEE(
                glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "Pingpong FBO {} is not complete!", i
                );
    }
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void BloomEffect::resize(int w, int h) {
    // ponovo kreiraj sve pri resize
    initialize(w, h, m_blur_shader, m_final_shader);
}

void BloomEffect::begin() {
    // prebaci render u sceneFBO
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_scene_fbo);
    GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    CHECKED_GL_CALL(glDrawBuffers, 2, bufs);
    CHECKED_GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BloomEffect::end(const glm::vec2 &vp) {
    CHECKED_GL_CALL(glDisable, GL_DEPTH_TEST);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    bool horizontal = true;
    bool firstPass = true;
    const int blurPasses = 10;

    // blur iteracije gaus
    m_blur_shader->use();
    for (int i = 0; i < blurPasses; ++i) {
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[horizontal]);
        m_blur_shader->set_int("horizontal", horizontal);
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, firstPass ? m_bright_tex : m_pingpong_tex[!horizontal]);
        draw_quad();
        horizontal = !horizontal;
        if (firstPass) firstPass = false;
    }
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    CHECKED_GL_CALL(glViewport, 0, 0, (int)vp.x, (int)vp.y);
    RG_GUARANTEE(
            m_final_shader,
            "Final shader not initialized"
            );
    m_final_shader->use();
    m_final_shader->set_int("scene", 0);
    m_final_shader->set_int("bloomBlur", 1);
    m_final_shader->set_float("bloomIntensity", 1.0f);

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_scene_color_tex);
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE1);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_tex[!horizontal]);
    draw_quad();

    CHECKED_GL_CALL(glEnable, GL_DEPTH_TEST);
}

void BloomEffect::draw_quad() {
    if (m_quad_vao == 0) {
        float verts[] = {
                // positions // texture Coords
                -1.0f, 1.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 1.0f, 0.0f,
        };

        CHECKED_GL_CALL(glGenVertexArrays, 1, &m_quad_vao);
        CHECKED_GL_CALL(glGenBuffers, 1, &m_quad_vbo);
        CHECKED_GL_CALL(glBindVertexArray, m_quad_vao);
        CHECKED_GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, m_quad_vbo);
        CHECKED_GL_CALL(glBufferData, GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        CHECKED_GL_CALL(glEnableVertexAttribArray, 0);
        CHECKED_GL_CALL(glVertexAttribPointer, 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        CHECKED_GL_CALL(glEnableVertexAttribArray, 1);
        CHECKED_GL_CALL(glVertexAttribPointer, 1, 2, GL_FLOAT, GL_FALSE,
                        4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    CHECKED_GL_CALL(glBindVertexArray, m_quad_vao);
    CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
    CHECKED_GL_CALL(glBindVertexArray, 0);
}
