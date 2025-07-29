#include "engine/graphics/BloomEffect.hpp"
#include <engine/resources/Shader.hpp>
#include <glad/glad.h>
#include <engine/graphics/OpenGL.hpp>
#include <engine/util/Errors.hpp>

using namespace engine::graphics;

BloomEffect::BloomEffect() = default;

BloomEffect::~BloomEffect() {
    CHECKED_GL_CALL(glDeleteFramebuffers, 1, &sceneFBO_);
    CHECKED_GL_CALL(glDeleteTextures, 1, &sceneColorTex_);
    CHECKED_GL_CALL(glDeleteTextures, 1, &brightTex_);
    CHECKED_GL_CALL(glDeleteRenderbuffers, 1, &depthRBO_);
    for (int i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glDeleteFramebuffers, 1, &pingpongFBO_[i]);
        CHECKED_GL_CALL(glDeleteTextures, 1, &pingpongTex_[i]);
    }
    if (quadVAO_) CHECKED_GL_CALL(glDeleteVertexArrays, 1, &quadVAO_);
    if (quadVBO_) CHECKED_GL_CALL(glDeleteBuffers,1, &quadVBO_);
}

void BloomEffect::initialize(int w, int h, resources::Shader* blurShader,resources::Shader* finalShader) {
    width_ = w;
    height_ = h;
    blurShader_ = blurShader;
    finalShader_ = finalShader;


    CHECKED_GL_CALL(glGenFramebuffers, 1, &sceneFBO_);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, sceneFBO_);

    // glavna boja
    CHECKED_GL_CALL(glGenTextures, 1, &sceneColorTex_);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, sceneColorTex_);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex_, 0);

    // samo svetli pikseli
    CHECKED_GL_CALL(glGenTextures, 1, &brightTex_);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, brightTex_);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightTex_,  0);

    // rbo
    CHECKED_GL_CALL(glGenRenderbuffers, 1, &depthRBO_);
    CHECKED_GL_CALL(glBindRenderbuffer, GL_RENDERBUFFER, depthRBO_);
    CHECKED_GL_CALL(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    CHECKED_GL_CALL(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO_);


    GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    CHECKED_GL_CALL(glDrawBuffers, 2, bufs);

    RG_GUARANTEE(
       glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
       "Framebuffer not complete!"
   );
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    //ping-pong
    for (int i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glGenFramebuffers, 1, &pingpongFBO_[i]);
        CHECKED_GL_CALL(glGenTextures, 1, &pingpongTex_[i]);
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, pingpongFBO_[i]);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, pingpongTex_[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongTex_[i], 0);
        RG_GUARANTEE(
           glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
           "Pingpong FBO {} is not complete!", i
       );
    }
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void BloomEffect::resize(int w, int h) {
    // ponovo kreiraj sve pri resize
    initialize(w, h, blurShader_, finalShader_);
}

void BloomEffect::begin() {
    // prebaci render u sceneFBO
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, sceneFBO_);
    GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    CHECKED_GL_CALL(glDrawBuffers, 2, bufs);
    CHECKED_GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BloomEffect::end(const glm::vec2& vp) {
    CHECKED_GL_CALL(glDisable, GL_DEPTH_TEST);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    bool horizontal = true;
    bool firstPass = true;
    const int blurPasses = 10;

    // blur iteracije gaus
    blurShader_->use();
    for (int i = 0; i < blurPasses; ++i) {
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, pingpongFBO_[horizontal]);
        blurShader_->set_int("horizontal", horizontal);
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, firstPass ? brightTex_ : pingpongTex_[!horizontal]);
        drawQuad();
        horizontal = !horizontal;
        if (firstPass) firstPass = false;
    }
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    CHECKED_GL_CALL(glViewport, 0, 0, (int)vp.x, (int)vp.y);
    RG_GUARANTEE(
        finalShader_,
        "Final shader not initialized"
    );
    finalShader_->use();
    finalShader_->set_int("scene", 0);
    finalShader_->set_int("bloomBlur", 1);
    finalShader_->set_float("bloomIntensity", 1.0f);

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, sceneColorTex_);
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE1);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, pingpongTex_[!horizontal]);
    drawQuad();

    CHECKED_GL_CALL(glEnable, GL_DEPTH_TEST);
}

void BloomEffect::drawQuad() {
    if (quadVAO_ == 0) {
        float verts[] = {
            // positions // texture Coords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
        };

        CHECKED_GL_CALL(glGenVertexArrays, 1, &quadVAO_);
        CHECKED_GL_CALL(glGenBuffers, 1, &quadVBO_);
        CHECKED_GL_CALL(glBindVertexArray, quadVAO_);
        CHECKED_GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, quadVBO_);
        CHECKED_GL_CALL(glBufferData, GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        CHECKED_GL_CALL(glEnableVertexAttribArray, 0);
        CHECKED_GL_CALL(glVertexAttribPointer, 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        CHECKED_GL_CALL(glEnableVertexAttribArray, 1);
        CHECKED_GL_CALL(glVertexAttribPointer, 1, 2, GL_FLOAT, GL_FALSE,
                        4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    CHECKED_GL_CALL(glBindVertexArray, quadVAO_);
    CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
    CHECKED_GL_CALL(glBindVertexArray, 0);
}
