// gl_objects.hpp: move-only RAII owners for OpenGL buffer and vertex-array names.
// Must be destroyed while the GL context that created them is still current.
#pragma once

#include "render/gl.hpp"

namespace pfl {

class Buffer {
public:
    Buffer() = default;
    ~Buffer();
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    static Buffer create();
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

class VertexArray {
public:
    VertexArray() = default;
    ~VertexArray();
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    static VertexArray create();
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

} // namespace pfl
