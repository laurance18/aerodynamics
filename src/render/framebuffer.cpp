#include "render/framebuffer.hpp"

namespace pfl {

Framebuffer::~Framebuffer() { release(); }

void Framebuffer::release() {
    if (m_texture != 0) glDeleteTextures(1, &m_texture);
    if (m_fbo != 0) glDeleteFramebuffers(1, &m_fbo);
    m_texture = 0;
    m_fbo = 0;
    m_width = 0;
    m_height = 0;
}

bool Framebuffer::resize(int width, int height) {
    if (width == m_width && height == m_height && m_fbo != 0) return false;
    release();

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    // 16-bit float so the repeated fade multiply decays to zero instead of leaving 8-bit ghosts.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_width = width;
    m_height = height;
    return true;
}

} // namespace pfl
