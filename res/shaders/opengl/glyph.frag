#version 460 core

layout(binding = 0) uniform sampler2D glyph_sdf;

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 tex_coord;

void main() {
    vec4 c = texture(glyph_sdf, tex_coord);
    
    //float r = c.r - 0.5;

    /*

    float dx = dFdx(r);
    float dy = dFdy(r);

    if(r > 0.0) {
        float rnx = r - dx;
        float rpx = r + dx;
        float rny = r - dy;
        float rpy = r + dy;

        float mr = min(min(rnx, rpx), min(rny, rpy));

        if(mr < 0.0) {
            float rr = mr / (mr - r);

            frag_color = vec4(rr, rr, rr, 1.0);
        } else if(r + dx < 0.0) {
            frag_color = vec4(0.0, 0.0, 0.0, 1.0);
        }
    } else {
        frag_color = vec4(1.0, 1.0, 1.0, 1.0);
    }
    */

    frag_color = c;

    //if(r > 0.0) frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    //else frag_color = vec4(1.0);
}