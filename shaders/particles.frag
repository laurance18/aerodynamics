#version 330 core
// Round, anti-aliased points coloured by speed (slow = blue, fast = red).

in float v_t;
out vec4 o_color;

vec3 speedColormap(float t) {
    const vec3 c0 = vec3(0.15, 0.35, 0.95);
    const vec3 c1 = vec3(0.20, 0.85, 0.90);
    const vec3 c2 = vec3(0.95, 0.90, 0.30);
    const vec3 c3 = vec3(1.00, 0.35, 0.20);
    if (t < 1.0 / 3.0) return mix(c0, c1, t * 3.0);
    if (t < 2.0 / 3.0) return mix(c1, c2, t * 3.0 - 1.0);
    return mix(c2, c3, t * 3.0 - 2.0);
}

void main() {
    vec2 d = gl_PointCoord - vec2(0.5);
    float r2 = dot(d, d);
    if (r2 > 0.25) discard;
    float alpha = 1.0 - smoothstep(0.12, 0.25, r2);
    o_color = vec4(speedColormap(v_t), alpha);
}
