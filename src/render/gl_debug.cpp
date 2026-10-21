#include "render/gl_debug.hpp"

#include "render/gl.hpp"

#include <cstdio>

namespace pfl {
namespace {

void GLAD_API_PTR onGlDebugMessage(GLenum /*source*/, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/,
                                   const GLchar* message, const void* /*user*/) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    const char* level = severity == GL_DEBUG_SEVERITY_HIGH     ? "HIGH"
                        : severity == GL_DEBUG_SEVERITY_MEDIUM ? "MEDIUM"
                                                               : "LOW";
    const char* kind = type == GL_DEBUG_TYPE_ERROR ? "error" : "message";
    std::fprintf(stderr, "[GL %s %s %u] %s\n", level, kind, id, message);
}

} // namespace

bool installGlDebugOutput() {
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) == 0) return false;
    if (!(GLAD_GL_VERSION_4_3 || GLAD_GL_KHR_debug) || glDebugMessageCallback == nullptr) return false;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(onGlDebugMessage, nullptr);
    return true;
}

} // namespace pfl
