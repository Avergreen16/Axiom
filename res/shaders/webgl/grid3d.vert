#version 300 es

precision highp float;

layout(location = 0) in vec2 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec2 coords0;
out mat4 inv_model;
out mat4 inv_view;
out mat4 inv_proj;

void main() {
    gl_Position = vec4(position, 0.5, 1.0);

    inv_model = inverse(model);
    inv_view = inverse(view);
    inv_proj = inverse(proj);

    coords0 = (inv_view * inv_proj * vec4(position, 0.0, 1.0)).xy;
}