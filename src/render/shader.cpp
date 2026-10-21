#include "render/shader.hpp"

#include "render/shader_sources.hpp"

#include <utility>

namespace pfl {
namespace {

std::string shaderInfoLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length > 0 ? length : 0), '\0');
    if (length > 0) glGetShaderInfoLog(shader, length, nullptr, log.data());
    while (!log.empty() && (log.back() == '\0' || log.back() == '\n')) log.pop_back();
    return log;
}

std::string programInfoLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length > 0 ? length : 0), '\0');
    if (length > 0) glGetProgramInfoLog(program, length, nullptr, log.data());
    while (!log.empty() && (log.back() == '\0' || log.back() == '\n')) log.pop_back();
    return log;
}

GLuint compileStage(GLenum stage, std::string_view label, std::string_view source, std::string& error) {
    const GLuint shader = glCreateShader(stage);
    const char* text = source.data();
    const GLint length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &text, &length);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        const char* stageName = stage == GL_VERTEX_SHADER ? "vertex" : "fragment";
        error += "Shader compile error (" + std::string(stageName) + " shader, " + std::string(label) +
                 "):\n" + shaderInfoLog(shader) + "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

ShaderProgram::~ShaderProgram() {
    if (m_id != 0) glDeleteProgram(m_id);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : m_id(std::exchange(other.m_id, 0)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteProgram(m_id);
        m_id = std::exchange(other.m_id, 0);
    }
    return *this;
}

ShaderProgram ShaderProgram::build(std::string_view label, std::string_view vertexSource,
                                   std::string_view fragmentSource, std::string& error) {
    const GLuint vs = compileStage(GL_VERTEX_SHADER, label, vertexSource, error);
    const GLuint fs = compileStage(GL_FRAGMENT_SHADER, label, fragmentSource, error);
    if (vs == 0 || fs == 0) {
        if (vs != 0) glDeleteShader(vs);
        if (fs != 0) glDeleteShader(fs);
        return {};
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        error += "Shader link error (" + std::string(label) + "):\n" + programInfoLog(program) + "\n";
        glDeleteProgram(program);
        return {};
    }
    return ShaderProgram(program);
}

ShaderProgram ShaderProgram::load(std::string_view vertexName, std::string_view fragmentName, std::string& error) {
    const auto vs = loadShaderSource(vertexName);
    const auto fs = loadShaderSource(fragmentName);
    if (!vs) error += "Shader source not found: " + std::string(vertexName) + "\n";
    if (!fs) error += "Shader source not found: " + std::string(fragmentName) + "\n";
    if (!vs || !fs) return {};

    const std::string label = std::string(vertexName) + " + " + std::string(fragmentName);
    return build(label, *vs, *fs, error);
}

void ShaderProgram::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name), value);
}

void ShaderProgram::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name), value);
}

void ShaderProgram::setVec2(const char* name, float x, float y) const {
    glUniform2f(glGetUniformLocation(m_id, name), x, y);
}

} // namespace pfl
