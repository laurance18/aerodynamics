#version 330 core
// Composites the (premultiplied-alpha) trail buffer onto the screen.

in vec2 v_uv;
uniform sampler2D u_trail;
out vec4 o_color;

void main() {
    o_color = texture(u_trail, v_uv);
}
