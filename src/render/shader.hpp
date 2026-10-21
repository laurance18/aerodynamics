// shader.hpp: GLSL program compilation/linking with full error logs, plus uniform helpers.
#pragma once

#include "render/gl.hpp"

#include <string>
#include <string_view>

namespace pfl {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Compiles and links a vertex + fragment program. On failure returns an invalid program and
    // writes a readable report (stage, label, driver info log) to `error`.
    static ShaderProgram build(std::string_view label, std::string_view vertexSource,
                               std::string_view fragmentSource, std::string& error);

    // Loads "<vertexName>" and "<fragmentName>" via shader_sources.hpp, then build().
    static ShaderProgram load(std::string_view vertexName, std::string_view fragmentName, std::string& error);

    bool valid() const { return m_id != 0; }
    GLuint id() const { return m_id; }
    void use() const { glUseProgram(m_id); }

    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, float x, float y) const;

private:
    explicit ShaderProgram(GLuint id) : m_id(id) {}
    GLuint m_id = 0;
};

} // namespace pfl
