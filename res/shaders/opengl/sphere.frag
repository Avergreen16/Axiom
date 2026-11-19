#version 460 core

layout(binding = 0) uniform sampler2D cubemap;

layout(location = 0) out vec4 frag_color;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) in vec2 cs;
layout(location = 1) in mat4 inv_model;
layout(location = 5) in mat4 inv_view;
layout(location = 9) in mat4 inv_proj;

//

mat3 face_matrices[6] = {
    mat3(vec3(0, 1, 0), vec3(0, 0, 1), vec3(1, 0, 0)),
    mat3(vec3(0, -1, 0), vec3(0, 0, 1), vec3(-1, 0, 0)),
    mat3(vec3(-1, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0)),
    mat3(vec3(1, 0, 0), vec3(0, 0, 1), vec3(0, -1, 0)),
    mat3(vec3(0, 1, 0), vec3(-1, 0, 0), vec3(0, 0, 1)),
    mat3(vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 0, -1))
};

vec2 face_tex_coords[6] = {
    vec2(1, 1) / vec2(4, 3),
    vec2(3, 1) / vec2(4, 3),
    vec2(2, 1) / vec2(4, 3),
    vec2(0, 1) / vec2(4, 3),
    vec2(1, 2) / vec2(4, 3),
    vec2(1, 0) / vec2(4, 3)
};

in vec2 tex_coord;
in mat4 inverse_proj;
in mat4 inverse_model;

vec3 light_dir = normalize(vec3(-1.0, 1.0, 1.0));

void main() {
    vec4 cc = vec4(cs, 0.5, 1.0);
    cc = inv_proj * cc;
    cc /= cc.w;
    cc = inv_view * cc;

    vec3 ray = normalize(cc.xyz);
    vec3 origin = vec3(model * vec4(0.0, 0.0, 0.0, 1.0));
    
    float sphere_dist = dot(origin, ray);
    float len = length(ray * sphere_dist - origin);

    float t = sqrt(1.0 - len * len);

    float surface_dist = sphere_dist - t;

    vec3 pos = ray * surface_dist - origin;

    //

    vec4 d = proj * (view * vec4((ray * surface_dist), 1.0));
    d /= d.w;
    gl_FragDepth = d.z;

    vec2 tex_coord;

    if(abs(pos.x) >= abs(pos.y) && abs(pos.x) >= abs(pos.z)) {
        vec3 square = pos / abs(pos.x);
        if(pos.x > 0.0) {
            mat3 inv_f = inverse(face_matrices[0]);
            square = inv_f * square;

            tex_coord = square.xy * 0.5 + 0.5;
            tex_coord = tex_coord / vec2(4, 3) + face_tex_coords[0];
        } else {
            mat3 inv_f = inverse(face_matrices[1]);
            square = inv_f * square;

            tex_coord = square.xy * 0.5 + 0.5;
            tex_coord = tex_coord / vec2(4, 3) + face_tex_coords[1];
        }
    } else if(abs(pos.y) > abs(pos.x) && abs(pos.y) >= abs(pos.z)) {
        vec3 square = pos / abs(pos.y);
        if(pos.y > 0.0) {
            mat3 inv_f = inverse(face_matrices[2]);
            square = inv_f * square;

            tex_coord = square.xy * 0.5 + 0.5;
            tex_coord = tex_coord / vec2(4, 3) + face_tex_coords[2];
        } else {
            mat3 inv_f = inverse(face_matrices[3]);
            square = inv_f * square;

            tex_coord = square.xy * 0.5 + 0.5;
            tex_coord = tex_coord / vec2(4, 3) + face_tex_coords[3];
        }
    } else if(abs(pos.z) > abs(pos.x) && abs(pos.z) > abs(pos.y)) {
        vec3 square = pos / abs(pos.z);
        if(pos.z > 0.0) {
            mat3 inv_f = inverse(face_matrices[4]);
            square = inv_f * square;

            tex_coord = square.xy * 0.5 + 0.5;
            tex_coord = tex_coord / vec2(4, 3) + face_tex_coords[4];
        } else {
            mat3 inv_f = inverse(face_matrices[5]);
            square = inv_f * square;

            tex_coord = square.xy * 0.5 + 0.5;
            tex_coord = tex_coord / vec2(4, 3) + face_tex_coords[5];
        }
    }

    vec3 normal = normalize(pos);
    vec3 ldir = inverse(mat3(view)) * light_dir;

    float s = clamp(dot(normal, ldir), 0.0, 1.0);

    frag_color.w = 1.0;
    frag_color.xyz = texture(cubemap, tex_coord).xyz * s;
}