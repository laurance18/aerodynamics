// framebuffer.hpp: RAII framebuffer object with a single RGBA16F colour texture.
#pragma once

#include "render/gl.hpp"

namespace pfl {

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // (Re)allocates storage when the size changes. Returns true if the buffer was recreated
    // (its contents are then cleared to transparent black).
    bool resize(int width, int height);

    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }
    GLuint texture() const { return m_texture; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    void release();

    GLuint m_fbo = 0;
    GLuint m_texture = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace pfl
