namespace axiom {

void push_uniform(uint index, int value) {
    glUniform1i(index, value);
}
void push_uniform(uint index, uint value) {
    glUniform1ui(index, value);
}

void push_uniform(uint index, float value) {
    glUniform1f(index, value);
}

void push_uniform(uint index, ivec2 value) {
    glUniform2i(index, value.x, value.y);
}

void push_uniform(uint index, uvec2 value) {
    glUniform2ui(index, value.x, value.y);
}

void push_uniform(uint index, vec2 value) {
    glUniform2f(index, value.x, value.y);
}

//

void push_uniform(uint index, ivec3 value) {
    glUniform3i(index, value.x, value.y, value.z);
}

void push_uniform(uint index, uvec3 value) {
    glUniform3ui(index, value.x, value.y, value.z);
}

void push_uniform(uint index, vec3 value) {
    glUniform3f(index, value.x, value.y, value.z);
}

//

void push_uniform(uint index, ivec4 value) {
    glUniform4i(index, value.x, value.y, value.z, value.w);
}

void push_uniform(uint index, uvec4 value) {
    glUniform4ui(index, value.x, value.y, value.z, value.w);
}

void push_uniform(uint index, vec4 value) {
    glUniform4f(index, value.x, value.y, value.z, value.w);
}

// pointers

void push_uniform(uint index, int* value) {
    glUniform1iv(index, 1, value);
}

void push_uniform(uint index, uint* value) {
    glUniform1uiv(index, 1, value);
}

void push_uniform(uint index, float* value) {
    glUniform1fv(index, 1, value);
}

//

void push_uniform(uint index, ivec2* value) {
    glUniform2iv(index, 1, (int*)value);
}

void push_uniform(uint index, uvec2* value) {
    glUniform2uiv(index, 1, (uint*)value);
}

void push_uniform(uint index, vec2* value) {
    glUniform2fv(index, 1, (float*)value);
}

//

void push_uniform(uint index, ivec3* value) {
    glUniform3iv(index, 1, (int*)value);
}

void push_uniform(uint index, uvec3* value) {
    glUniform3uiv(index, 1, (uint*)value);
}

void push_uniform(uint index, vec3* value) {
    glUniform3fv(index, 1, (float*)value);
}

//

void push_uniform(uint index, ivec4* value) {
    glUniform4iv(index, 1, (int*)value);
}

void push_uniform(uint index, uvec4* value) {
    glUniform4uiv(index, 1, (uint*)value);
}

void push_uniform(uint index, vec4* value) {
    glUniform4fv(index, 1, (float*)value);
}

//

void push_uniform(uint index, mat2* value) {
    glUniformMatrix2fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, glm::mat2x3* value) {
    glUniformMatrix2x3fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, glm::mat2x4* value) {
    glUniformMatrix2x4fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, glm::mat3x2* value) {
    glUniformMatrix3x2fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, mat3* value) {
    glUniformMatrix3fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, glm::mat3x4* value) {
    glUniformMatrix3x4fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, glm::mat4x2* value) {
    glUniformMatrix4x2fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, glm::mat4x3* value) {
    glUniformMatrix4x3fv(index, 1, false, (const float*)value);
}

void push_uniform(uint index, mat4* value) {
    glUniformMatrix4fv(index, 1, false, (const float*)value);
}

// pointer packs

void push_uniform(uint index, uint count, int* value) {
    glUniform1iv(index, count, value);
}

void push_uniform(uint index, uint count, uint* value) {
    glUniform1uiv(index, count, value);
}

void push_uniform(uint index, uint count, float* value) {
    glUniform1fv(index, count, value);
}

//

void push_uniform(uint index, uint count, ivec2* value) {
    glUniform2iv(index, count, (int*)value);
}

void push_uniform(uint index, uint count, uvec2* value) {
    glUniform2uiv(index, count, (uint*)value);
}

void push_uniform(uint index, uint count, vec2* value) {
    glUniform2fv(index, count, (float*)value);
}

//

void push_uniform(uint index, uint count, ivec3* value) {
    glUniform3iv(index, count, (int*)value);
}

void push_uniform(uint index, uint count, uvec3* value) {
    glUniform3uiv(index, count, (uint*)value);
}

void push_uniform(uint index, uint count, vec3* value) {
    glUniform3fv(index, count, (float*)value);
}

//

void push_uniform(uint index, uint count, ivec4* value) {
    glUniform4iv(index, count, (int*)value);
}

void push_uniform(uint index, uint count, uvec4* value) {
    glUniform4uiv(index, count, (uint*)value);
}

void push_uniform(uint index, uint count, vec4* value) {
    glUniform4fv(index, count, (float*)value);
}

//

void push_uniform(uint index, uint count, mat2* value) {
    glUniformMatrix2fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, glm::mat2x3* value) {
    glUniformMatrix2x3fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, glm::mat2x4* value) {
    glUniformMatrix2x4fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, glm::mat3x2* value) {
    glUniformMatrix3x2fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, mat3* value) {
    glUniformMatrix3fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, glm::mat3x4* value) {
    glUniformMatrix3x4fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, glm::mat4x2* value) {
    glUniformMatrix4x2fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, glm::mat4x3* value) {
    glUniformMatrix4x3fv(index, count, false, (const float*)value);
}

void push_uniform(uint index, uint count, mat4* value) {
    glUniformMatrix4fv(index, count, false, (const float*)value);
}

}