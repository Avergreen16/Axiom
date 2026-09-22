#version 460 core

vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 coords;

vec3 hex_color(uint i) {
    return vec3((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF) / float(0xFF);
}

float filtered_grid(vec2 p, vec2 dpdx, vec2 dpdy, float blend) {
    const vec2 N = max(vec2(6.0 * blend + (6.0 * 4.0) * (1.0 - blend)), 1.0 / (max(abs(dpdx), abs(dpdy)) * 0.5));
    vec2 w = max(abs(dpdx), abs(dpdy));
    vec2 a = p + 0.5 * w;                        
    vec2 b = p - 0.5 * w;           
    vec2 i = (floor(a) + min(fract(a) * N, 1.0) - floor(b) - min(fract(b) * N, 1.0)) / (N * w);
    return (1.0 - i.x) * (1.0 - i.y);
}

vec3 base_color = hex_color(0x1E1F2E);
vec3 sector_color = hex_color(0xFF893D);

void main() {
    vec4 c = vec4(base_color, 1.0);

    int line_width = 1;

    vec2 dx = dFdx(coords);
    vec2 dy = dFdy(coords);
    
    float ddx = length(vec2(dx.x, dy.x));
    float ddy = length(vec2(dx.y, dy.y));

    vec4 line_c = vec4(0.0);
    vec4 line_a = vec4(0.0);

    float f = 1.0 - filtered_grid(coords, dx, dy, 0.5);

    c.rgb = vec3(1.0) * f + c.rgb * (1.0 - f);

    frag_color = c;
}