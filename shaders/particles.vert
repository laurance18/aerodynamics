#version 330 core
// Tracer particles: world-space position + speed -> clip space, point size, colormap coordinate.

layout(location = 0) in vec2 a_pos;
layout(location = 1) in float a_speed;

uniform vec2 u_viewCenter;     // world coordinates at the centre of the viewport
uniform vec2 u_viewHalfExtent; // world half-width / half-height visible in the viewport
uniform float u_speedScale;    // 1 / speed that maps to the top of the colormap
uniform float u_pointSize;     // pixels

out float v_t;

void main() {
    gl_Position = vec4((a_pos - u_viewCenter) / u_viewHalfExtent, 0.0, 1.0);
    gl_PointSize = u_pointSize;
    v_t = clamp(a_speed * u_speedScale, 0.0, 1.0);
}
