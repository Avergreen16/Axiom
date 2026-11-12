#pragma once

namespace axiom {
    
template<typename type>
inline void push_uniform(uint index, type value);

template<typename type>
inline void push_uniform(uint index, type* value);

template<typename type>
inline void push_uniform(uint index, uint count, type* value);

// by value

template<>
inline void push_uniform(uint index, int value) {
    glUniform1i(index, value);
}

template<>
inline void push_uniform(uint index, uint value) {
    glUniform1ui(index, value);
}

template<>
inline void push_uniform(uint index, float value) {
    glUniform1f(index, value);
}

template<>
inline void push_uniform(uint index, double value) {
    glUniform1d(index, value);
}

//

template<>
inline void push_uniform(uint index, ivec2 value) {
    glUniform2i(index, value.x, value.y);
}

template<>
inline void push_uniform(uint index, uvec2 value) {
    glUniform2ui(index, value.x, value.y);
}

template<>
inline void push_uniform(uint index, vec2 value) {
    glUniform2f(index, value.x, value.y);
}

template<>
inline void push_uniform(uint index, dvec2 value) {
    glUniform2d(index, value.x, value.y);
}

//

template<>
inline void push_uniform(uint index, ivec3 value) {
    glUniform3i(index, value.x, value.y, value.z);
}

template<>
inline void push_uniform(uint index, uvec3 value) {
    glUniform3ui(index, value.x, value.y, value.z);
}

template<>
inline void push_uniform(uint index, vec3 value) {
    glUniform3f(index, value.x, value.y, value.z);
}

template<>
inline void push_uniform(uint index, dvec3 value) {
    glUniform3d(index, value.x, value.y, value.z);
}

//

template<>
inline void push_uniform(uint index, ivec4 value) {
    glUniform4i(index, value.x, value.y, value.z, value.w);
}

template<>
inline void push_uniform(uint index, uvec4 value) {
    glUniform4ui(index, value.x, value.y, value.z, value.w);
}

template<>
inline void push_uniform(uint index, vec4 value) {
    glUniform4f(index, value.x, value.y, value.z, value.w);
}

template<>
inline void push_uniform(uint index, dvec4 value) {
    glUniform4d(index, value.x, value.y, value.z, value.w);
}

// pointers

template<>
inline void push_uniform(uint index, int* value) {
    glUniform1iv(index, 1, value);
}

template<>
inline void push_uniform(uint index, uint* value) {
    glUniform1uiv(index, 1, value);
}

template<>
inline void push_uniform(uint index, float* value) {
    glUniform1fv(index, 1, value);
}

template<>
inline void push_uniform(uint index, double* value) {
    glUniform1dv(index, 1, value);
}

//

template<>
inline void push_uniform(uint index, ivec2* value) {
    glUniform2iv(index, 1, (int*)value);
}

template<>
inline void push_uniform(uint index, uvec2* value) {
    glUniform2uiv(index, 1, (uint*)value);
}

template<>
inline void push_uniform(uint index, vec2* value) {
    glUniform2fv(index, 1, (float*)value);
}

template<>
inline void push_uniform(uint index, dvec2* value) {
    glUniform2dv(index, 1, (double*)value);
}

//

template<>
inline void push_uniform(uint index, ivec3* value) {
    glUniform3iv(index, 1, (int*)value);
}

template<>
inline void push_uniform(uint index, uvec3* value) {
    glUniform3uiv(index, 1, (uint*)value);
}

template<>
inline void push_uniform(uint index, vec3* value) {
    glUniform3fv(index, 1, (float*)value);
}

template<>
inline void push_uniform(uint index, dvec3* value) {
    glUniform3dv(index, 1, (double*)value);
}

//

template<>
inline void push_uniform(uint index, ivec4* value) {
    glUniform4iv(index, 1, (int*)value);
}

template<>
inline void push_uniform(uint index, uvec4* value) {
    glUniform4uiv(index, 1, (uint*)value);
}

template<>
inline void push_uniform(uint index, vec4* value) {
    glUniform4fv(index, 1, (float*)value);
}

template<>
inline void push_uniform(uint index, dvec4* value) {
    glUniform4dv(index, 1, (double*)value);
}

//

template<>
inline void push_uniform(uint index, mat2* value) {
    glUniformMatrix2fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, glm::mat2x3* value) {
    glUniformMatrix2x3fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, glm::mat2x4* value) {
    glUniformMatrix2x4fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, glm::mat3x2* value) {
    glUniformMatrix3x2fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, mat3* value) {
    glUniformMatrix3fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, glm::mat3x4* value) {
    glUniformMatrix3x4fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, glm::mat4x2* value) {
    glUniformMatrix4x2fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, glm::mat4x3* value) {
    glUniformMatrix4x3fv(index, 1, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, mat4* value) {
    glUniformMatrix4fv(index, 1, false, (const float*)value);
}

// pointer packs

template<>
inline void push_uniform(uint index, uint count, int* value) {
    glUniform1iv(index, count, value);
}

template<>
inline void push_uniform(uint index, uint count, uint* value) {
    glUniform1uiv(index, count, value);
}

template<>
inline void push_uniform(uint index, uint count, float* value) {
    glUniform1fv(index, count, value);
}

template<>
inline void push_uniform(uint index, uint count, double* value) {
    glUniform1dv(index, count, value);
}

//

template<>
inline void push_uniform(uint index, uint count, ivec2* value) {
    glUniform2iv(index, count, (int*)value);
}

template<>
inline void push_uniform(uint index, uint count, uvec2* value) {
    glUniform2uiv(index, count, (uint*)value);
}

template<>
inline void push_uniform(uint index, uint count, vec2* value) {
    glUniform2fv(index, count, (float*)value);
}

template<>
inline void push_uniform(uint index, uint count, dvec2* value) {
    glUniform2dv(index, count, (double*)value);
}

//

template<>
inline void push_uniform(uint index, uint count, ivec3* value) {
    glUniform3iv(index, count, (int*)value);
}

template<>
inline void push_uniform(uint index, uint count, uvec3* value) {
    glUniform3uiv(index, count, (uint*)value);
}

template<>
inline void push_uniform(uint index, uint count, vec3* value) {
    glUniform3fv(index, count, (float*)value);
}

template<>
inline void push_uniform(uint index, uint count, dvec3* value) {
    glUniform3dv(index, count, (double*)value);
}

//

template<>
inline void push_uniform(uint index, uint count, ivec4* value) {
    glUniform4iv(index, count, (int*)value);
}

template<>
inline void push_uniform(uint index, uint count, uvec4* value) {
    glUniform4uiv(index, count, (uint*)value);
}

template<>
inline void push_uniform(uint index, uint count, vec4* value) {
    glUniform4fv(index, count, (float*)value);
}

template<>
inline void push_uniform(uint index, uint count, dvec4* value) {
    glUniform4dv(index, count, (double*)value);
}

//

template<>
inline void push_uniform(uint index, uint count, mat2* value) {
    glUniformMatrix2fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, glm::mat2x3* value) {
    glUniformMatrix2x3fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, glm::mat2x4* value) {
    glUniformMatrix2x4fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, glm::mat3x2* value) {
    glUniformMatrix3x2fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, mat3* value) {
    glUniformMatrix3fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, glm::mat3x4* value) {
    glUniformMatrix3x4fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, glm::mat4x2* value) {
    glUniformMatrix4x2fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, glm::mat4x3* value) {
    glUniformMatrix4x3fv(index, count, false, (const float*)value);
}

template<>
inline void push_uniform(uint index, uint count, mat4* value) {
    glUniformMatrix4fv(index, count, false, (const float*)value);
}


}