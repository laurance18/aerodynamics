// particle_renderer.hpp: streams CPU particle state to a VBO each frame and draws GL_POINTS.
#pragma once

#include "core/particles.hpp"
#include "core/vec2.hpp"
#include "render/gl_objects.hpp"
#include "render/shader.hpp"

#include <span>
#include <string>
#include <vector>

namespace pfl {

struct ParticleView {
    Vec2 center;              // world point at the viewport centre
    Vec2 halfExtent{1.0, 1.0}; // world half-size of the viewport
    float speedScale = 1.0f;  // 1 / speed mapped to the top of the colormap
    float pointSize = 2.0f;   // pixels
};

class ParticleRenderer {
public:
    bool init(std::string& error);
    // Keeps the previous program if the new one fails to build.
    bool reloadShaders(std::string& error);

    void upload(std::span<const Particle> particles);
    void draw(const ParticleView& view) const;

private:
    ShaderProgram m_program;
    VertexArray m_vao;
    Buffer m_vbo;
    std::vector<float> m_staging; // x, y, speed per particle
    GLsizei m_count = 0;
    std::size_t m_capacityBytes = 0;
};

} // namespace pfl
