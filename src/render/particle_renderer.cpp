#include "render/particle_renderer.hpp"

namespace pfl {

bool ParticleRenderer::init(std::string& error) {
    if (!reloadShaders(error)) return false;

    m_vao = VertexArray::create();
    m_vbo = Buffer::create();
    glBindVertexArray(m_vao.id());
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
    constexpr GLsizei stride = 3 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool ParticleRenderer::reloadShaders(std::string& error) {
    ShaderProgram program = ShaderProgram::load("particles.vert", "particles.frag", error);
    if (!program.valid()) return false;
    m_program = std::move(program);
    return true;
}

void ParticleRenderer::upload(std::span<const Particle> particles) {
    m_staging.resize(particles.size() * 3);
    for (std::size_t i = 0; i < particles.size(); ++i) {
        m_staging[3 * i + 0] = static_cast<float>(particles[i].pos.x);
        m_staging[3 * i + 1] = static_cast<float>(particles[i].pos.y);
        m_staging[3 * i + 2] = static_cast<float>(particles[i].speed);
    }
    const auto bytes = static_cast<GLsizeiptr>(m_staging.size() * sizeof(float));

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
    if (static_cast<std::size_t>(bytes) != m_capacityBytes) {
        glBufferData(GL_ARRAY_BUFFER, bytes, m_staging.data(), GL_STREAM_DRAW);
        m_capacityBytes = static_cast<std::size_t>(bytes);
    } else {
        // Orphan the old storage so the driver doesn't stall on the previous frame's draw.
        glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, m_staging.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_count = static_cast<GLsizei>(particles.size());
}

void ParticleRenderer::draw(const ParticleView& view) const {
    if (m_count == 0) return;
    m_program.use();
    m_program.setVec2("u_viewCenter", static_cast<float>(view.center.x), static_cast<float>(view.center.y));
    m_program.setVec2("u_viewHalfExtent", static_cast<float>(view.halfExtent.x),
                      static_cast<float>(view.halfExtent.y));
    m_program.setFloat("u_speedScale", view.speedScale);
    m_program.setFloat("u_pointSize", view.pointSize);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    // Colour: normal "over"; alpha: accumulate coverage. Leaves premultiplied colour in the
    // trail buffer, which the composite pass blends with GL_ONE / GL_ONE_MINUS_SRC_ALPHA.
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_vao.id());
    glDrawArrays(GL_POINTS, 0, m_count);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

} // namespace pfl
