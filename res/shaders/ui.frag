#version 460 core

struct clip_space {
    vec4 range;
    float radius;
    uint parent;
};

layout(std430, binding = 0) buffer ClipSpaces {
    clip_space clips[];
};

layout(binding = 0) uniform sampler2D text_texture;
layout(binding = 1) uniform sampler2D gui_texture;
layout(binding = 2) uniform sampler2D render_texture[16];

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec4 color;
layout(location = 2) flat in uint data;
layout(location = 3) in vec4 range;
layout(location = 4) in vec2 position;

out vec4 frag_color;

float sdf(vec2 pos, clip_space clip) {
    vec2 center = (clip.range.xy + clip.range.zw) * 0.5;
    vec2 half_size = clip.range.zw - center;

    vec2 d = abs(pos - center) - half_size + clip.radius;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - clip.radius; 
}

void main() {
    clip_space cc = clips[0];

    if(position.x < range.x || position.x > range.z || position.y < range.y || position.y > range.w) discard;
    else {
        vec4 c;

        uint d = data;

        if((d & 0x80000000) != 0x0) {
            uint index = (d & ~0x80000000);
            c = texture(render_texture[index], tex_coord);
        } else {
            if((d & 0x1u) == 0x0u) {
                c = texture(text_texture, tex_coord / textureSize(text_texture, 0));
            } else {
                if((d & 0x2u) == 0x0u) c = texture(gui_texture, tex_coord / textureSize(gui_texture, 0));
                /*
                else {
                    uint target = (d >> 2) & 0xF;
                    c = texture(targets, tex_coord);
                }
                */
            }
        }
        frag_color = c * color;

        frag_color = mix(frag_color, vec4(0.0), clamp(sdf(position, cc), 0.0, 1.0));
    }
}