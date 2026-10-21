// trail_renderer.hpp: fading-trail effect. Particles are drawn into an off-screen buffer that
// is multiplied by (1 - fade) every frame, then the buffer is composited onto the screen.
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

    // Binds the trail buffer (resizing it if needed) and applies the fade pass.
    void begin(int width, int height, float fade);
    // Binds the default framebuffer and blends the trail buffer over it.
    void composite(int width, int height) const;
    // Empties the trail buffer (e.g. when trails are switched back on).
    void clear();

private:
    ShaderProgram m_fade;
    ShaderProgram m_composite;
    VertexArray m_emptyVao; // core profile needs a VAO bound even for attribute-less draws
    Framebuffer m_buffer;
};

} // namespace pfl
