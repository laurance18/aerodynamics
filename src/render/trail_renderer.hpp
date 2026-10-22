// trail_renderer.hpp: off-screen particle layer with an optional fading-trail effect.
//
// Particles are drawn into an off-screen buffer that is multiplied by (1 - fade) every frame
// (fade = 1 clears it, i.e. no trails). The buffer's texture holds premultiplied colour and is
// shown by the UI inside the plot, so it must be blended with GL_ONE / GL_ONE_MINUS_SRC_ALPHA.
#pragma once

#include "render/framebuffer.hpp"
#include "render/gl_objects.hpp"
#include "render/shader.hpp"

#include <string>

namespace pfl {

class TrailRenderer {
public:
    bool init(std::string& error);
    bool reloadShaders(std::string& error);

    // (Re)allocates the buffer; returns true if it was recreated, which also changes texture().
    // Call it before the UI records draw commands that use texture() in the same frame.
    bool resize(int width, int height) { return m_buffer.resize(width, height); }
    // Binds the trail buffer, sets the viewport to it and applies the fade.
    void begin(float fade);
    // Binds the default framebuffer again.
    void end() const;
    // Empties the trail buffer (e.g. when the view or the flow changes).
    void clear();

    GLuint texture() const { return m_buffer.texture(); }
    int width() const { return m_buffer.width(); }
    int height() const { return m_buffer.height(); }

private:
    ShaderProgram m_fade;
    VertexArray m_emptyVao; // core profile needs a VAO bound even for attribute-less draws
    Framebuffer m_buffer;
};

} // namespace pfl
