#version 330 core
// Trail fade pass. Drawn with glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA), so every channel
// of the trail buffer is multiplied by (1 - u_fade) each frame.

uniform float u_fade;
out vec4 o_color;

void main() {
    o_color = vec4(0.0, 0.0, 0.0, u_fade);
}
