#version 300 es

precision highp float;

uniform sampler2D base_texture;
uniform sampler2D text_texture;

in vec2 vert_pos;

layout(location = 0) out vec4 frag_color;

vec3 rgb_constants = vec3(0.2126, 0.7152, 0.0722);

void main() {
    vec4 bcol = texture(base_texture, vert_pos);
    vec4 tcol = texture(text_texture, vert_pos);

    vec3 tcol2 = tcol.xyz;// / rgb_constants;
    float m = max(max(tcol2.x, tcol2.y), tcol2.z);
    //if(m > 1.0) tcol2 /= m;
    
    vec4 self_col = bcol;
    self_col.r = mix(self_col.r, 1.0, tcol2.r);
    self_col.g = mix(self_col.g, 1.0, tcol2.g);
    self_col.b = mix(self_col.b, 1.0, tcol2.b);
    self_col.a = 1.0;

    //frag_color = self_col;
    //frag_color.w = 1.0;

    vec2 v = vec2(vert_pos * 500.0) / vec2(textureSize(base_texture, 0));

    frag_color = self_col;
}