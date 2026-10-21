#include "render/gl_objects.hpp"

#include <utility>

namespace pfl {

Buffer::~Buffer() {
    if (m_id != 0) glDeleteBuffers(1, &m_id);
}

Buffer::Buffer(Buffer&& other) noexcept : m_id(std::exchange(other.m_id, 0)) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteBuffers(1, &m_id);
        m_id = std::exchange(other.m_id, 0);
    }
    return *this;
}

Buffer Buffer::create() {
    Buffer b;
    glGenBuffers(1, &b.m_id);
    return b;
}

VertexArray::~VertexArray() {
    if (m_id != 0) glDeleteVertexArrays(1, &m_id);
}

VertexArray::VertexArray(VertexArray&& other) noexcept : m_id(std::exchange(other.m_id, 0)) {}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (m_id != 0) glDeleteVertexArrays(1, &m_id);
        m_id = std::exchange(other.m_id, 0);
    }
    return *this;
}

VertexArray VertexArray::create() {
    VertexArray v;
    glGenVertexArrays(1, &v.m_id);
    return v;
}

} // namespace pfl
