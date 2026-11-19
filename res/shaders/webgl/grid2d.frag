#version 300 es

precision highp float;

layout(location = 0) out vec4 frag_color;

in vec2 coords0;
in mat4 inv_view;
in mat4 inv_proj;

uniform ivec2 size;
uniform uint num_touches;
uniform vec2 touches[20];

vec3 hex_color(uint r, uint g, uint b) {
    return vec3(float(r), float(g), float(b)) / float(0xFF);
}

float F(float x, float width) {
    float f = (x * x - max(x - width, 0.0) * max(x - width, 0.0)) / (x * x);
    return f;
}

float grid(vec2 coords, float width) {
    vec2 dx = dFdx(coords);
    vec2 dy = dFdy(coords);
    float dd = max(length(dx),length(dy));
    float m = max(max(abs(dx.x), abs(dx.y)), max(abs(dy.x), abs(dy.y)));
    float ddd = max(1.0, 1.0 / m);

    float frac = F(16.0, width) / F(ddd, width);
    float w = frac;

    //

    vec2 v0 = coords + vec2(dx + dy) * float(width) * 0.5;
    vec2 v1 = coords + vec2(dx + -dy) * float(width) * 0.5;
    vec2 v2 = coords + vec2(-dx + dy) * float(width) * 0.5;
    vec2 v3 = coords + vec2(-dx + -dy) * float(width) * 0.5;

    vec2 direction = normalize(vec2(1.0, 0.0));
    vec2 d = vec2(direction.y, -direction.x);
    float f0 = floor(dot(d, v0));
    float f1 = floor(dot(d, v1));
    float f2 = floor(dot(d, v2));
    float f3 = floor(dot(d, v3));

    int fmod = int(fract((f0 + f1 + f2 + f3) / 4.0) * 4.0);
    bool is_two = fmod == 2;
    bool is_three = fmod == 3;
    bool is_m = abs(f0 - f1) > 1.0 || abs(f1 - f2) > 1.0 || abs(f2 - f3) > 1.0 || abs(f3 - f0) > 1.0 || abs(f0 - f2) > 1.0 || abs(f1 - f3) > 1.0;

    if(is_two || is_three || is_m) return clamp(w, 0.0, 1.0);
    
    direction = normalize(vec2(0.0, 1.0));
    d = vec2(direction.y, -direction.x);
    f0 = floor(dot(d, v0));
    f1 = floor(dot(d, v1));
    f2 = floor(dot(d, v2));
    f3 = floor(dot(d, v3));

    fmod = int(fract((f0 + f1 + f2 + f3) / 4.0) * 4.0);
    is_two = fmod == 2;
    is_three = fmod == 3;
    is_m = abs(f0 - f1) > 1.0 || abs(f1 - f2) > 1.0 || abs(f2 - f3) > 1.0 || abs(f3 - f0) > 1.0 || abs(f0 - f2) > 1.0 || abs(f1 - f3) > 1.0;

    if(is_two || is_three || is_m) return clamp(w, 0.0, 1.0);

    return 0.0;
}

bool line(vec2 coords, vec2 origin, vec2 direction, int width) {
    vec2 rel = coords - origin;

    vec2 dx = dFdx(rel);
    vec2 dy = dFdy(rel);

    //

    vec2 v0 = rel + vec2(dx + dy) * float(width) * 0.5;
    vec2 v1 = rel + vec2(dx + -dy) * float(width) * 0.5;
    vec2 v2 = rel + vec2(-dx + dy) * float(width) * 0.5;
    vec2 v3 = rel + vec2(-dx + -dy) * float(width) * 0.5;

    vec2 d = vec2(direction.y, -direction.x);
    float f0 = floor(dot(d, v0));
    float f1 = floor(dot(d, v1));
    float f2 = floor(dot(d, v2));
    float f3 = floor(dot(d, v3));

    int fmod = int(f0 >= 0.0) + int(f1 >= 0.0) + int(f2 >= 0.0) + int(f3 >= 0.0);
    bool is_two = fmod == 2;
    bool is_three = fmod == 3;
    
    if(is_two || is_three) return true;
    return false;
}

vec4 blend(vec4 dst, vec4 src) {
    return vec4(dst.xyz * (1.0 - src.a) + src.xyz * src.a, dst.a + (1.0 - dst.a) * src.a);
}

void main() {
    vec3 base_color = hex_color(uint(0x1E), uint(0x1F), uint(0x2E));
    vec3 sector_color = hex_color(uint(0xFF), uint(0x89), uint(0x3D));

    vec2 coords = gl_FragCoord.xy;
    coords /= vec2(size);
    coords = coords * 2.0 - 1.0;

    coords = vec2(inv_view * inv_proj * vec4(coords, 0.0, 1.0));
    coords += 0.00001;
    
    vec2 dx = dFdx(coords);
    vec2 dy = dFdy(coords);
    float dd = max(length(dx),length(dy));

    float g = 0.0;
    float l = log(dd) / log(16.0);
    int ll = int(floor(l));
    int start = max(0, ll);
    int end = max(0, ll + 2);

    for(int i = start; i <= end; ++i) {
        float is_grid = grid(coords / pow(16.0, float(i)), 1.0);

        g = max(g, is_grid);
    }

    bool is_xline = line(coords, vec2(0.0), vec2(1.0, 0.0), 2);
    bool is_yline = line(coords, vec2(0.0), vec2(0.0, 1.0), 2);

    g = min(1.0, g);
    frag_color = vec4(base_color, 1.0);

    vec4 c0 = vec4(base_color * 2.0, g);
    frag_color = blend(frag_color, c0);

    if(is_xline) frag_color = vec4(hex_color(0xFFu, 0x59u, 0x59u), 1.0);
    else if(is_yline) frag_color = vec4(hex_color(0x59u, 0xFFu, 0x59u), 1.0);
}