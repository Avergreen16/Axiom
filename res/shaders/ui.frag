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
layout(location = 3) flat in uint clip;
layout(location = 4) in vec2 position;

layout(location = 1) out vec4 base_color;
layout(location = 2) out vec4 text_color;

float sdf(vec2 pos, clip_space clip) {
    vec2 center = (clip.range.xy + clip.range.zw) * 0.5;
    vec2 half_size = clip.range.zw - center;

    vec2 d = abs(pos - center) - half_size + abs(clip.radius);
    float c = min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - abs(clip.radius);
    if(clip.radius < 0.0) c = -c;
    
    return c; 
}

void main() {
    float sdfc = 1.0;
    uint c = clip;
    while(c != 0xFFFFFFFF) {
        sdfc *= clamp(0.5 - sdf(position, clips[c]), 0.0, 1.0);
        c = clips[c].parent;
    }

    vec4 cc;

    uint d = data;

    bool text = false;

    if((d & 0x80000000) != 0x0) {
        uint index = (d & ~0x80000000);
        cc = texture(render_texture[index], tex_coord);
    } else {
        if((d & 0x1u) == 0x0u) {
            cc = texture(text_texture, tex_coord / textureSize(text_texture, 0));
            text = true;
        } else {
            if((d & 0x2u) == 0x0u) cc = texture(gui_texture, tex_coord / textureSize(gui_texture, 0));
            /*
            else {
                uint target = (d >> 2) & 0xF;
                c = texture(targets, tex_coord);
            }
            */
        }
    }

    if(text) {
        //vec4 self_col = texelFetch(self, ivec2(position), 0);
        //self_col.r = mix(self_col.r, color.r, cc.r * sdfc);
        //self_col.g = mix(self_col.g, color.g, cc.g * sdfc);
        //self_col.b = mix(self_col.b, color.b, cc.b * sdfc);

        //frag_color = self_col;
        //frag_color.w = 1.0;

        text_color = cc;
        text_color.w *= sdfc;
    } else {
        base_color = cc * color;
        base_color.w *= sdfc;
        
        text_color = vec4(0.0);
    }
}