#version 300 es

precision highp float;
precision highp int;

/*
struct clip_space {
    vec4 range;
    float radius;
    uint parent;
};

layout(std140) uniform ClipSpaces {
    clip_space clips[256];
};
*/

uniform sampler2D text_texture;
uniform sampler2D ui_texture;
uniform sampler2D render_texture[6];

in vec2 vert_tex;
in vec4 vert_color;
flat in uint vert_data;
flat in uint vert_clip;
in vec2 vert_pos;

layout(location = 0) out vec4 base_color;
layout(location = 1) out vec4 text_color;

/*
float sdf(vec2 pos, clip_space clip) {
    vec2 center = (clip.range.xy + clip.range.zw) * 0.5;
    vec2 half_size = clip.range.zw - center;

    vec2 d = abs(pos - center) - half_size;// + abs(clip.radius);
    float c = min(max(d.x, d.y), 0.0) + length(max(d, 0.0));// - abs(clip.radius);
    if(clip.radius < 0.0) c = -c;
    
    return c; 
}
*/

vec4 sample_texture(uint index, vec2 uv) {
    switch(index) {
        case uint(0): return texture(render_texture[0], uv);
        case uint(1): return texture(render_texture[1], uv);
        case uint(2): return texture(render_texture[2], uv);
        case uint(3): return texture(render_texture[3], uv);
        case uint(4): return texture(render_texture[4], uv);
        case uint(5): return texture(render_texture[5], uv);
        /*
        case uint(6): return texture(render_texture[6], uv);
        case uint(7): return texture(render_texture[7], uv);
        case uint(8): return texture(render_texture[8], uv);
        case uint(9): return texture(render_texture[9], uv);
        case uint(10): return texture(render_texture[10], uv);
        case uint(11): return texture(render_texture[11], uv);
        case uint(12): return texture(render_texture[12], uv);
        case uint(13): return texture(render_texture[13], uv);
        case uint(14): return texture(render_texture[14], uv);
        case uint(15): return texture(render_texture[15], uv);
        */
    }

    return vec4(1.0, 1.0, 0.0, 1.0);
}

void main() {
    float sdfc = 1.0;
    /*
    uint c = vert_clip;
    while(c != uint(0xFFFFFFFF)) {
        sdfc *= clamp(0.5 - sdf(vert_pos, clips[c]), 0.0, 1.0);
        c = clips[c].parent;
    }
    */

    vec4 cc;

    uint d = vert_data;

    bool text = false;

    if((d & 0x80000000u) != 0x0u) {
        uint index = (d & ~0x80000000u);
        cc = texture(render_texture[0], vert_tex);//sample_texture(index, vert_tex);

        cc.w = 1.0;
    } else {
        if((d & 0x1u) == 0x0u) {
            cc = texture(text_texture, vert_tex / vec2(textureSize(text_texture, 0)));
            text = true;
        } else {
            if((d & 0x2u) == 0x0u) cc = texture(ui_texture, vert_tex / vec2(textureSize(ui_texture, 0)));
            
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
        base_color = cc * vert_color;
        base_color.w *= sdfc;

        //base_color = texture(render_texture0, vert_tex);
        
        text_color = vec4(0.0, 0.0, 0.0, base_color.w);
    }
}