#version 460 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(location = 3) uniform ivec2 viewport_size;

layout(location = 0) out vec3 color;
layout(location = 1) out float luminosity;
layout(location = 2) out vec2 tex;

layout(location = 0) in vec3 i_color[];
layout(location = 1) in float i_luminosity[];

void main() {    
    vec2 size = vec2(21.0) / vec2(viewport_size);
    vec4 position = gl_in[0].gl_Position;
    if(position.w > 0.0) {
        position /= position.w;

        position.xy = floor(position.xy * vec2(viewport_size * 0.5)) + 0.5;
        position.xy /= vec2(viewport_size * 0.5);


        gl_Position = position + vec4(-size.x, -size.y, 0.0, 0.0); 
        color = i_color[0];
        luminosity = i_luminosity[0];
        tex = vec2(-1.0, -1.0);

        EmitVertex();

        gl_Position = position + vec4(size.x, -size.y, 0.0, 0.0); 
        color = i_color[0];
        luminosity = i_luminosity[0];
        tex = vec2(1.0, -1.0);

        EmitVertex();
        
        gl_Position = position + vec4(-size.x, size.y, 0.0, 0.0); 
        color = i_color[0];
        luminosity = i_luminosity[0];
        tex = vec2(-1.0, 1.0);

        EmitVertex();

        gl_Position = position + vec4(size.x, size.y, 0.0, 0.0); 
        color = i_color[0];
        luminosity = i_luminosity[0];
        tex = vec2(1.0, 1.0);

        EmitVertex();
        
        EndPrimitive();
    }
}  
