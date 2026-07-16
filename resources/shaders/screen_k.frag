#version 460 core

layout(rgba32f, binding = 0) uniform image3D kbuffer;

ivec3 size = ivec3(imageSize(kbuffer));

out vec4 frag_color; 

void main() { 
    ivec2 pos = ivec2(gl_FragCoord.xy); 
    
    uint num = 0;

    /*
    bool f = true;
    for(int j = 0; j < size.z; ++j) {
        if(f) {
            ivec3 coord = ivec3(pos, j * 2);

            vec4 depth_normal = imageLoad(kbuffer, coord);

            if(depth_normal.x == -1.0) {
                f = false;
            } else {
                frag_color = imageLoad(kbuffer, coord + ivec3(0, 0, 1));
            }
        }
    }
    */

    vec4 v = imageLoad(kbuffer, ivec3(pos, 1));
    if(v.x != -1.0) frag_color = v;
}
