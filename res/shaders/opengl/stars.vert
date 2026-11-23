#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in float luminosity;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) out vec3 o_color;
layout(location = 1) out float o_luminosity;

mat4 inv_proj = inverse(proj);
vec4 v = inv_proj * vec4(1.0, 1.0, -1.0, 1.0) / (inv_proj * vec4(1.0, 1.0, -1.0, 1.0)).w;
vec2 angles = vec2(
    asin(abs(normalize(vec2(v.x, v.z)).x)), 
    asin(abs(normalize(vec2(v.y, v.z)).x))
);
void main() {
    vec3 vv = vec3((view * model) * vec4(normalize(position), 1.0));

    gl_Position = proj * vec4(vv, 1.0);
    /*
    if(gl_Position.w > 0.0) {
        vec2 angles_vv = vec2(
            asin(abs(normalize(vec2(vv.x, vv.z)).x)), 
            asin(abs(normalize(vec2(vv.y, vv.z)).x))
        );

        angles_vv = angles_vv / angles * sign(gl_Position.xy);

        gl_Position = vec4(angles_vv * gl_Position.w, gl_Position.z, gl_Position.w);
    }
    */

    o_color = color;
    o_luminosity = luminosity;
}