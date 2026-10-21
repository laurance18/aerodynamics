#version 330 core
// Full-screen triangle generated from gl_VertexID; draw 3 vertices with an empty VAO bound.

out vec2 v_uv;

void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    v_uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
