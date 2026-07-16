#version 460 core

vec2 vertices[4] = {
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5)
};

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 position[];
layout(location = 1) in vec4 color[];
layout(location = 2) in vec2 size[];
layout(location = 3) in vec2 tex_coord[];
layout(location = 4) in vec2 tex_size[];
layout(location = 5) in vec3 orient[];
layout(location = 6) flat in uint settings[];

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec2 o_tex_coord;

layout(location = 0) uniform mat4 view_mat;
layout(location = 1) uniform mat4 proj_mat;
layout(location = 2) uniform vec3 particle_up;
layout(location = 3) uniform ivec2 screen_size;

void main() {
    for(int i = 0; i < 4; ++i) {
        vec2 pos = vertices[i];

        vec2 tex_coord_c = tex_coord[0] + tex_size[0] * (pos + vec2(0.5));

        vec4 view_pos = view_mat * vec4(position[0], 1.0);

        pos *= size[0];

        vec3 x = vec3(1, 0, 0);
        vec3 y = vec3(0, 1, 0);
        float fade_factor = 1;

        if((settings[0] & 0x8) == 0x8) {
            clamp(-view_pos.z / (max(size[0].x, size[0].y) * 0.5), 0, 1);
        }

        vec4 col = color[0];
        col.a *= fade_factor;

        if((settings[0] & 0x10) == 0x10) {
            gl_Position = proj_mat * view_pos;
            gl_Position += vec4(pos.x / (screen_size.x * 0.5) * gl_Position.w, pos.y / (screen_size.y * 0.5) * gl_Position.w, 0.0, 0.0);
        } else if((settings[0] & 0x20) == 0x20) {
            vec3 down = mat3(view_mat) * orient[0];
            down = normalize(vec3(down.x, down.y, 0.0f));
            vec3 a = vec3(-down.y, down.x, 0.0f);
            vec3 b = -down;

            gl_Position = proj_mat * (view_pos + vec4(-pos.x * a - pos.y * b, 0.0));
        } else {
            gl_Position = proj_mat * (view_pos + vec4(pos.x, pos.y, 0.0, 0.0));   
        }

        if((settings[0] & 0x40) == 0x40) {
            if(gl_Position.w >= 0) {
                gl_Position /= gl_Position.w;
                gl_Position.z = 0.99999;
            }
        }

        o_color = col;
        o_tex_coord = tex_coord_c;

        EmitVertex();
    }

    EndPrimitive();
}