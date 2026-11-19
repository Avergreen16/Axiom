#version 460 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 4) out;

layout(binding = 0) uniform sampler2D viewport_normal_tex;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec3 o_normal;
layout(location = 2) out vec3 o_position;

layout(location = 0) in vec4 i_color[];
layout(location = 1) in vec3 i_position[];

vec3 get_normal(vec2 pos) {
    vec2 size = textureSize(viewport_normal_tex, 0);

    vec2 positions[] = {
        vec2(1, 0) / size,
        vec2(-1, 0) / size,
        vec2(0, 1) / size,
        vec2(0, -1) / size,
    };

    vec3 pos2[4];

    for(int i = 0; i < 4; ++i) {
        vec2 p = pos + positions[i];

        vec3 pp = vec3(p * size - size * 0.5, texture(viewport_normal_tex, clamp(p, 0.0, 0.999)).r * 256);

        pos2[i] = pp;
    }

    vec3 n0 = pos2[1] - pos2[0];
    vec3 n1 = pos2[3] - pos2[2];

    return normalize(cross(n0, n1));
}

void main() {  
    vec4 pos0 = vec4(gl_in[0].gl_Position.xy * 256.0 - 128.0, texture(viewport_normal_tex, clamp(i_position[0].xy, 0.0, 0.999)).r * 48.0, 1.0);
    vec4 pos1 = vec4(gl_in[1].gl_Position.xy * 256.0 - 128.0, texture(viewport_normal_tex, clamp(i_position[1].xy, 0.0, 0.999)).r * 48.0, 1.0);
    vec4 pos2 = vec4(gl_in[2].gl_Position.xy * 256.0 - 128.0, texture(viewport_normal_tex, clamp(i_position[2].xy, 0.0, 0.999)).r * 48.0, 1.0);

    vec3 normal = cross(pos0.xyz - pos2.xyz, pos1.xyz - pos2.xyz);
    normal = normalize(normal);

    gl_Position = proj * view * model * pos0;
    o_color = i_color[0];
    o_position = pos0.xyz;
    o_normal = get_normal(gl_in[0].gl_Position.xy);
    EmitVertex();
    
    gl_Position = proj * view * model * pos1;
    o_color = i_color[1];
    o_position = pos1.xyz;
    o_normal = get_normal(gl_in[1].gl_Position.xy);
    EmitVertex();
    
    gl_Position = proj * view * model * pos2;
    o_color = i_color[2];
    o_position = pos2.xyz;
    o_normal = get_normal(gl_in[2].gl_Position.xy);
    EmitVertex();

    EndPrimitive();
}  
