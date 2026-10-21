#include "render/trail_renderer.hpp"

namespace pfl {

bool TrailRenderer::init(std::string& error) {
    m_emptyVao = VertexArray::create();
    return reloadShaders(error);
}

bool TrailRenderer::reloadShaders(std::string& error) {
    ShaderProgram fade = ShaderProgram::load("fullscreen.vert", "fade.frag", error);
    if (!fade.valid()) return false;
    m_fade = std::move(fade);
    return true;
}

void TrailRenderer::begin(float fade) {
    m_buffer.bind();
    glViewport(0, 0, m_buffer.width(), m_buffer.height());

    m_fade.use();
    m_fade.setFloat("u_fade", fade);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_emptyVao.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void TrailRenderer::end() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void TrailRenderer::clear() {
    if (m_buffer.width() == 0) return;
    m_buffer.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace pfl
