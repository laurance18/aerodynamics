#include "render/trail_renderer.hpp"

namespace pfl {

bool TrailRenderer::init(std::string& error) {
    m_emptyVao = VertexArray::create();
    return reloadShaders(error);
}

bool TrailRenderer::reloadShaders(std::string& error) {
    ShaderProgram fade = ShaderProgram::load("fullscreen.vert", "fade.frag", error);
    ShaderProgram composite = ShaderProgram::load("fullscreen.vert", "composite.frag", error);
    if (!fade.valid() || !composite.valid()) return false;
    m_fade = std::move(fade);
    m_composite = std::move(composite);
    return true;
}

void TrailRenderer::begin(int width, int height, float fade) {
    m_buffer.resize(width, height);
    m_buffer.bind();
    glViewport(0, 0, width, height);

    m_fade.use();
    m_fade.setFloat("u_fade", fade);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_emptyVao.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void TrailRenderer::composite(int width, int height) const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    m_composite.use();
    m_composite.setInt("u_trail", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_buffer.texture());
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_emptyVao.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TrailRenderer::clear() {
    if (m_buffer.width() == 0) return;
    m_buffer.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace pfl
