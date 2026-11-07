#version 460 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 4) out;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec3 o_normal;
layout(location = 2) out vec3 o_position;
layout(location = 3) out vec2 o_texcoords;

layout(location = 0) in vec4 i_color[];
layout(location = 1) in vec3 i_normal[];
layout(location = 2) in vec3 i_position[];
layout(location = 3) in vec3 i_rposition[];

mat3 rotateAxis(vec3 axis, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat3(
        oc * axis.x * axis.x + c,
        oc * axis.x * axis.y + axis.z * s,
        oc * axis.z * axis.x - axis.y * s,

        oc * axis.x * axis.y - axis.z * s,
        oc * axis.y * axis.y + c,
        oc * axis.y * axis.z + axis.x * s,

        oc * axis.z * axis.x + axis.y * s,
        oc * axis.y * axis.z - axis.x * s,
        oc * axis.z * axis.z + c
    );
}

mat3 rotate_to(vec3 a, vec3 b) {
    vec3 cross_p = cross(a, b);
    float d = dot(a, b);
    float f = acos(d);
    float s = asin(length(cross_p));
    vec3 n = normalize(cross_p);
    
    mat3 matrix = rotateAxis(n, f);

    return matrix;
}

void main() { 
    vec3 normal = cross(i_rposition[0] - i_rposition[2], i_rposition[1] - i_rposition[2]);
    normal = normalize(normal);
    vec3 nn = normal / max(max(abs(normal.x), abs(normal.y)), abs(normal.z));
    nn = normalize(round(nn));

    mat3 rmat = rotate_to(nn, vec3(0.0, 0.0, 1.0));

    gl_Position = gl_in[0].gl_Position;
    o_color = i_color[0];
    o_normal = i_normal[0];
    o_position = i_position[0];
    o_texcoords = (rmat * i_rposition[0]).xy;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    o_color = i_color[1];
    o_normal = i_normal[1];
    o_position = i_position[1];
    o_texcoords = (rmat * i_rposition[1]).xy;
    EmitVertex();
    
    gl_Position = gl_in[2].gl_Position;
    o_color = i_color[2];
    o_normal = i_normal[2];
    o_position = i_position[2];
    o_texcoords = (rmat * i_rposition[2]).xy;
    EmitVertex();
    
    EndPrimitive();
}  
