#include <core/assets/assets.hpp>
#include <wrapper/wrapper.hpp>

#include <GLES3/gl3.h>

#include "stb_image.h"
#include "stb_image_write.h"

namespace axiom {
    
void set_clear_color(vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void set_clear_depth(float depth) {
    glClearDepthf(depth);
}

void enable_blend() {
    glEnable(GL_BLEND);
}

void disable_blend() {
    glDisable(GL_BLEND);
}

void enable_depth_test() {
    glEnable(GL_DEPTH_TEST);
}

void disable_depth_test() {
    glDisable(GL_DEPTH_TEST);
}

void enable_depth_clamp() {
    throw std::runtime_error("this function (enable_depth_clamp) is not supported by webGL");
}

void disable_depth_clamp() {
    throw std::runtime_error("this function (disable_depth_clamp) is not supported by webGL");
}

void set_viewport(ivec4 range) {
    glViewport(range.x, range.y, range.z, range.w);
}

void clear_framebuffer(bool clear_color, bool clear_depth, bool clear_stencil) {
    glClear((GL_COLOR_BUFFER_BIT * clear_color) | (GL_DEPTH_BUFFER_BIT * clear_depth) | (GL_STENCIL_BUFFER_BIT * clear_stencil));
}

void set_depth_func(axiom::depth_func depth_func) {
    uint d[] = {
        GL_NEVER, 
        GL_LESS, 
        GL_EQUAL,
        GL_LEQUAL, 
        GL_GREATER, 
        GL_NOTEQUAL, 
        GL_GEQUAL,
        GL_ALWAYS
    };

    glDepthFunc(d[(uint)depth_func]);
}

void set_depth_range(float near, float far) {
    glDepthRangef(near, far);
}

/*
void set_clip_control() {
    f
}
*/

void set_blend_func(blend_func src_func, blend_func dst_func) {
    uint opengl_values[] = {
        GL_ZERO, 
        GL_ONE, 
        GL_SRC_COLOR, 
        GL_ONE_MINUS_SRC_COLOR, 
        GL_DST_COLOR, 
        GL_ONE_MINUS_DST_COLOR, 
        GL_SRC_ALPHA, 
        GL_ONE_MINUS_SRC_ALPHA, 
        GL_DST_ALPHA, 
        GL_ONE_MINUS_DST_ALPHA, 
        GL_CONSTANT_COLOR, 
        GL_ONE_MINUS_CONSTANT_COLOR, 
        GL_CONSTANT_ALPHA, 
        GL_ONE_MINUS_CONSTANT_ALPHA, 
        GL_SRC_ALPHA_SATURATE, 
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
    };

    uint src = opengl_values[(uint)src_func];
    uint dst = opengl_values[(uint)dst_func];

    if(src == 0xFFFFFFFF) {
        throw std::runtime_error("set_blend_func: this (src_func) blend_func enum is not supported in WebGL");
    }
    
    if(dst == 0xFFFFFFFF) {
        throw std::runtime_error("set_blend_func: this (dst_func) blend_func enum is not supported in WebGL");
    }

    glBlendFunc(src, dst);
}

void set_blend_func_separate(blend_func src_func_color, blend_func dst_func_color, blend_func src_func_alpha, blend_func dst_func_alpha) {
    uint opengl_values[] = {
        GL_ZERO, 
        GL_ONE, 
        GL_SRC_COLOR, 
        GL_ONE_MINUS_SRC_COLOR, 
        GL_DST_COLOR, 
        GL_ONE_MINUS_DST_COLOR, 
        GL_SRC_ALPHA, 
        GL_ONE_MINUS_SRC_ALPHA, 
        GL_DST_ALPHA, 
        GL_ONE_MINUS_DST_ALPHA, 
        GL_CONSTANT_COLOR, 
        GL_ONE_MINUS_CONSTANT_COLOR, 
        GL_CONSTANT_ALPHA, 
        GL_ONE_MINUS_CONSTANT_ALPHA, 
        GL_SRC_ALPHA_SATURATE, 
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
    };

    uint src_c = opengl_values[(uint)src_func_color];
    uint dst_c = opengl_values[(uint)dst_func_color];
    uint src_a = opengl_values[(uint)src_func_alpha];
    uint dst_a = opengl_values[(uint)dst_func_alpha];

    if(src_c == 0xFFFFFFFF) throw std::runtime_error("set_blend_func_separate: this (src_func_color) blend_func enum is not supported in WebGL");
    if(dst_c == 0xFFFFFFFF) throw std::runtime_error("set_blend_func_separate: this (dst_func_color) blend_func enum is not supported in WebGL");
    if(src_a == 0xFFFFFFFF) throw std::runtime_error("set_blend_func_separate: this (src_func_alpha) blend_func enum is not supported in WebGL");
    if(dst_a == 0xFFFFFFFF) throw std::runtime_error("set_blend_func_separate: this (dst_func_alpha) blend_func enum is not supported in WebGL");

    glBlendFuncSeparate(src_c, dst_c, src_a, dst_a);
}

void bind_default_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

struct vertices::implementation {
    uint vertex_buffer;
    uint vertex_array;
    uint index_buffer;
    uint index_type;
};

vertices::vertices() : imp(new implementation) {

}

void set_blend() {
    
}

void blit(ivec4 src, ivec4 dst, framebuffer* srcf, framebuffer* dstf, axiom::texture_attachment src_attachment, axiom::texture_attachment dst_attachment, bool color, bool depth, bool stencil) {
    if(srcf == nullptr) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        glReadBuffer(get_texture_attachment(axiom::texture_attachment::COLOR0));
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcf->id);

        glReadBuffer(get_texture_attachment(src_attachment));
    }

    if(dstf == nullptr) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        uint buf[1] = {get_texture_attachment(axiom::texture_attachment::COLOR0)};
        glDrawBuffers(1, buf);
    } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstf->id);
        
        uint buf[1] = {get_texture_attachment(dst_attachment)};
        glDrawBuffers(1, buf);
    }

    glBlitFramebuffer(
        src.x, src.y, src.z, src.w, 
        dst.x, dst.y, dst.z, dst.w, 
        (GL_COLOR_BUFFER_BIT * color) | (GL_DEPTH_BUFFER_BIT * depth) | (GL_STENCIL_BUFFER_BIT * stencil),
        GL_NEAREST
    );
}

void vertices::init() {
    if(!initialized) {
        glGenBuffers(1, &imp->vertex_buffer);
        glGenVertexArrays(1, &imp->vertex_array);
        glBindVertexArray(imp->vertex_array);

        glGenBuffers(1, &imp->index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imp->index_buffer);

        initialized = true;
    }
}   

void vertices::vertex_buffer_data(void* ptr, uint num_vertices_, uint vertex_size, axiom::usage_type usage) {
    uint u[] = {
        GL_STATIC_DRAW,
        GL_DYNAMIC_DRAW,
        GL_STREAM_DRAW
    };

    bind();
    num_vertices = num_vertices_;
    glBindBuffer(GL_ARRAY_BUFFER, imp->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * vertex_size, ptr, u[(uint)usage]);
}

void vertices::index_buffer_data(void* ptr, uint num_indices_, axiom::number_type index_type_, uint index_size, axiom::usage_type usage) {
    uint v[] = {
        GL_BYTE, 
        GL_UNSIGNED_BYTE, 
        GL_SHORT, 
        GL_UNSIGNED_SHORT, 
        GL_INT, 
        GL_UNSIGNED_INT,

        GL_HALF_FLOAT, 
        GL_FLOAT, 
        0xFFFFFFFF, //GL_DOUBLE, 
        GL_INT_2_10_10_10_REV,
        GL_UNSIGNED_INT_2_10_10_10_REV
    };
    
    uint u[] = {
        GL_STATIC_DRAW,
        GL_DYNAMIC_DRAW,
        GL_STREAM_DRAW
    };

    if(v[(uint)index_type_] == 0xFFFFFFFF) throw std::runtime_error("index_buffer_data: this (index_type_) number_type enum is not supported in WebGL");

    bind();
    num_indices = num_indices_;
    imp->index_type = v[(uint)index_type_];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imp->index_buffer); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * index_size, ptr, u[(uint)usage]);
}

void vertices::add_vertex_attribute(uint index, uint size, axiom::number_type type, uint normalized, uint stride, uint offset) {
    uint v[] = {
        GL_BYTE, 
        GL_UNSIGNED_BYTE, 
        GL_SHORT, 
        GL_UNSIGNED_SHORT, 
        GL_INT, 
        GL_UNSIGNED_INT,

        GL_HALF_FLOAT, 
        GL_FLOAT, 
        0xFFFFFFFF, //GL_DOUBLE, 
        GL_INT_2_10_10_10_REV,
        GL_UNSIGNED_INT_2_10_10_10_REV
    };

    uint t = v[(uint)type];

    if(t == 0xFFFFFFFF) throw std::runtime_error("add_vertex_attribute: this (type) number_type enum is not supported in WebGL");

    glBindVertexArray(imp->vertex_array);
    if(t == GL_BYTE || t == GL_UNSIGNED_BYTE || t == GL_SHORT || t == GL_UNSIGNED_SHORT || t == GL_INT || t == GL_UNSIGNED_INT) glVertexAttribIPointer(index, size, t, stride, (void*)offset);
    else glVertexAttribPointer(index, size, t, normalized, stride, (void*)offset);
    glEnableVertexAttribArray(index);  
}

void vertices::bind() {
    glBindBuffer(GL_ARRAY_BUFFER, imp->vertex_buffer);
    glBindVertexArray(imp->vertex_array);
}

void vertices::draw_vertices_points() {
    bind();
    glDrawArrays(GL_POINTS, 0, num_vertices);
}

void vertices::draw_vertices_lines() {
    bind();
    glDrawArrays(GL_LINES, 0, num_vertices);
}

void vertices::draw_vertices_triangles() {
    bind();
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
}

void vertices::draw_indices_points() {
    bind();
    glDrawElements(GL_POINTS, num_indices, imp->index_type, (void*)0);
}

void vertices::draw_indices_lines() {
    bind();
    glDrawElements(GL_LINES, num_indices, imp->index_type, (void*)0);
}

void vertices::draw_indices_triangles() {
    bind();
    glDrawElements(GL_TRIANGLES, num_indices, imp->index_type, (void*)0);
}

vertices::vertices(vertices&& a) {
    imp->vertex_buffer = a.imp->vertex_buffer;
    a.imp->vertex_buffer = 0;

    imp->vertex_array = a.imp->vertex_array;
    a.imp->vertex_array = 0;

    imp->index_buffer = a.imp->index_buffer;
    a.imp->index_buffer = 0;

    num_indices = a.num_indices;
    a.num_indices = 0;

    num_vertices = a.num_vertices;
    a.num_vertices = 0;

    initialized = a.initialized;
    a.initialized = false;

    imp->index_type = a.imp->index_type;
}

vertices& vertices::operator=(vertices&& a) {
    imp->vertex_buffer = a.imp->vertex_buffer;
    a.imp->vertex_buffer = 0;

    imp->vertex_array = a.imp->vertex_array;
    a.imp->vertex_array = 0;

    imp->index_buffer = a.imp->index_buffer;
    a.imp->index_buffer = 0;

    num_indices = a.num_indices;
    a.num_indices = 0;

    num_vertices = a.num_vertices;
    a.num_vertices = 0;

    initialized = a.initialized;
    a.initialized = false;

    imp->index_type = a.imp->index_type;

    return *this;
}

vertices::~vertices() {
    if(initialized) {
        glDeleteBuffers(1, &imp->vertex_buffer);
        glDeleteBuffers(1, &imp->index_buffer);
        glDeleteVertexArrays(1, &imp->vertex_array);
    }
}

//

shader::shader(text_asset vertex_shader, text_asset fragment_shader) {
    compile(vertex_shader, fragment_shader);
}

shader::shader(shader&& s) {
    id = s.id;
    s.id = 0;

    uniforms = s.uniforms;
}

shader& shader::operator=(shader&& s) {
    id = s.id;
    s.id = 0;
    
    uniforms = s.uniforms;

    return *this;
}

shader::~shader() {
    glDeleteProgram(id);
}

bool shader::compile(text_asset vertex_shader, text_asset fragment_shader) {
    const char* v_ptr = vertex_shader.data.data();
    const char* f_ptr = fragment_shader.data.data();

    uint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &v_ptr, 0);
    glCompileShader(vs);
    
    // check if vertex shader compiled correctly
    int compile_check;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(vs, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << vertex_shader.filepath << "\n";
        std::cout << "ERROR: Vertex shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << vertex_shader.filepath << "\n";

        return false;
    }

    uint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &f_ptr, 0);
    glCompileShader(fs);
    
    // check if fragment shader compiled correctly
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(fs, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << fragment_shader.filepath << "\n";
        std::cout << "ERROR: Fragment shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << fragment_shader.filepath << "\n";

        glDeleteShader(vs);
        return false;
    }
    
    // attach and link vertex/fragment shaders
    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    glGetProgramiv(id, GL_LINK_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetProgramInfoLog(id, log_size, &log_size, &error_log[0]);
        std::cout << "ERROR: Program failed to link:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << fragment_shader.filepath << "\n";
        //std::cout << "Filepath: " << vertex_shader.filepath << "\n";

        glDeleteProgram(id);
        return false;
    }

    //

    GLint num_active_uniforms = 0;
    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_active_uniforms);

    for (GLint i = 0; i < num_active_uniforms; ++i) {
        GLchar name[256];
        GLint length;
        GLsizei size;
        GLenum type;

        // Get metadata and name
        glGetActiveUniform(id, (GLuint)i, sizeof(name) - 1, &length, &size, &type, name);
        name[length] = '\0';

        // Get the specific location
        GLint location = glGetUniformLocation(id, name);
        
        // Store or use 'location' and 'name'
        std::cout << "Uniform: " << name << " | Location: " << location << std::endl;

        uniforms[name] = location;
    }

    // delete vertex/fragment shaders after compilation
    glDeleteShader(vs);
    glDeleteShader(fs);

    return true;
}

void shader::use() {
    glUseProgram(id);
}

/*

*/

void uniform_buffer::buffer_data(void* data, uint size_bytes, uint usage) {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferData(GL_UNIFORM_BUFFER, size_bytes, data, usage);
}

void uniform_buffer::buffer_subdata(void* data, uint size_bytes, uint offset) {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size_bytes, data);
}

void uniform_buffer::bind(uint binding) {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
}

void uniform_buffer::delete_buffer() {
    glDeleteBuffers(1, &id);
}

uniform_buffer::~uniform_buffer() {
    delete_buffer();
}

uniform_buffer::uniform_buffer(uniform_buffer&& a) {
    id = a.id;
    a.id = 0;
}

uniform_buffer& uniform_buffer::operator=(uniform_buffer&& a) {
    id = a.id;
    a.id = 0;

    return *this;
}

texture::texture(texture_asset& asset, texture_format format, int mip_levels) {
    load(asset, format, mip_levels);
}

void texture::load(texture_asset& asset, texture_format format, int mip_levels) {
    type = GL_TEXTURE_2D;
    if(id != 0xFFFFFFFF) glDeleteTextures(1, &id);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    this->format = format;

    size.x = asset.size.x;
    size.y = asset.size.y;
    num_channels = asset.num_channels;
    uint8_t* data = asset.data.data();

    texture_desc desc = get_texture_desc(format);

    //

    glTexStorage2D(GL_TEXTURE_2D, mip_levels + 1, desc.formatbits, size.x, size.y);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, desc.format, desc.bits, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

bool texture::load(glm::uvec2 size, texture_format format) {
    type = GL_TEXTURE_2D;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    this->format = format;
    
    texture_desc desc = get_texture_desc(format);

    glTexStorage2D(GL_TEXTURE_2D, 1, desc.formatbits, size.x, size.y);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return true;
}

bool texture::load(glm::uvec3 size, texture_format format) {
    type = GL_TEXTURE_3D;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_3D, id);
    this->format = format;
    
    texture_desc desc = get_texture_desc(format);

    glTexStorage3D(GL_TEXTURE_3D, 1, desc.formatbits, size.x, size.y, size.z);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return true;
}

texture::texture(uint8_t* data, glm::uvec3 size_, uint type_, texture_format format, int mip_levels) {
    type = type_;
    size = size_;
    this->format = format;
    
    texture_desc desc = get_texture_desc(format);

    glGenTextures(1, &id);
    glBindTexture(type, id);

    if(!data) {
        std::cout << "ERROR: Texture failed to load. (raw data texture)\n";
        return;
    } else {

        if(this->type == GL_TEXTURE_2D) {
            glTexStorage2D(GL_TEXTURE_2D, mip_levels + 1, desc.formatbits, size.x, size.y);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, desc.format, desc.bits, data);
            
            if(mip_levels > 0) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, mip_levels);
                glGenerateMipmap(GL_TEXTURE_2D);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }
        } else if(this->type == GL_TEXTURE_3D) {
            glTexStorage3D(GL_TEXTURE_3D, mip_levels + 1, desc.formatbits, size.x, size.y, size.z);
            glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, size.x, size.y, size.z, desc.format, desc.bits, data);

            if(mip_levels == 0) {
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            } else {
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            }
        }

        return;
    }
}

texture::texture(glm::uvec3 size, uint type, texture_format format) {
    this->type = type;
    this->size = size;
    this->format = format;
    
    texture_desc desc = get_texture_desc(format);

    glGenTextures(1, &id);
    glBindTexture(type, id);

    if(type == GL_TEXTURE_2D) {
        glTexStorage2D(GL_TEXTURE_2D, 1, desc.formatbits, size.x, size.y);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else if(type == GL_TEXTURE_3D) {
        glTexStorage3D(GL_TEXTURE_3D, 4, desc.formatbits, size.x, size.y, size.z);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    } else if(this->type == GL_TEXTURE_2D_ARRAY) {
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, desc.formatbits, size.x, size.y, size.z);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
}

void texture::bind(int binding) {
    glActiveTexture(GL_TEXTURE0 + binding);
    glBindTexture(type, id);
}

void texture::bind() {
    glBindTexture(type, id);
}

texture::texture(texture&& t) noexcept {
    id = t.id;
    size = t.size;
    num_channels = t.num_channels;
    format = t.format;
    t.id = 0;
    type = t.type;
}

texture& texture::operator=(texture&& t) noexcept {
    id = t.id;
    size = t.size;
    num_channels = t.num_channels;
    format = t.format;
    t.id = 0;
    type = t.type;

    return *this;
}

void texture::delete_texture() {
    glDeleteTextures(1, &id);
}

texture::~texture() {
    glDeleteTextures(1, &id);
}

framebuffer::framebuffer(glm::ivec2 size_, std::vector<fb_tex_params> tp, axiom::texture_filter filter) {
    size = size_;
    tex_params = std::move(tp);
    this->filter = filter;

    initialized = true;
    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    for(int i = 0; i < tex_params.size(); ++i) {

        fb_tex_params& p = tex_params[i];

        textures.emplace_back(texture());
        texture& t = textures[i];

        glm::ivec2 s = size;

        texture_desc desc = get_texture_desc(p.format);

        uint fil;
        if(filter == axiom::texture_filter::NEAREST) fil = GL_NEAREST;
        else if(filter == axiom::texture_filter::LINEAR) fil = GL_LINEAR;

        if(p.layers > 1) {

            glGenTextures(1, &t.id);
            glBindTexture(GL_TEXTURE_2D_ARRAY, t.id);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, desc.formatbits, size.x, size.y, p.layers);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, fil);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, fil);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            t.size = glm::ivec3(size, p.layers);
            t.format = p.format;
            t.type = GL_TEXTURE_2D_ARRAY;
        } else {
            glGenTextures(1, &t.id);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glTexStorage2D(GL_TEXTURE_2D, 1, desc.formatbits, size.x, size.y);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, fil);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, fil);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            t.size = glm::ivec3(size, 1);
            t.format = p.format;
            t.type = GL_TEXTURE_2D;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, get_texture_attachment(p.attachment), GL_TEXTURE_2D, t.id, 0);

        bool insert = true;
        if(p.attachment == texture_attachment::DEPTH || p.attachment == texture_attachment::STENCIL || p.attachment == texture_attachment::DEPTH_STENCIL) {
            insert = false;

            if(p.attachment == texture_attachment::DEPTH || p.attachment == texture_attachment::DEPTH_STENCIL) depth_texture = i;
        }

        if(p.binding != -1 && insert) {
            int buffers_size = draw_buffers.size();

            if(p.binding >= buffers_size) {
                for(int j = 0; j < p.binding - buffers_size + 1; ++j) {
                    draw_buffers.push_back(GL_NONE);
                }
            }

            draw_buffers[p.binding] = get_texture_attachment(p.attachment);
        }
    }

    uint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) std::cerr << "FBO incomplete: " << status << std::endl;
}

framebuffer::framebuffer(framebuffer&& a) noexcept {
    id = a.id;
    a.id = 0;
    textures = std::move(a.textures);
    tex_params = std::move(a.tex_params);
    size = a.size;
    depth_texture = a.depth_texture;

    draw_buffers = a.draw_buffers;
}

framebuffer& framebuffer::operator=(framebuffer&& a) noexcept {
    id = a.id;
    a.id = 0;
    textures = std::move(a.textures);
    tex_params = std::move(a.tex_params);
    size = a.size;

    draw_buffers = a.draw_buffers;
    depth_texture = a.depth_texture;

    return *this;
}

void framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    
    glDrawBuffers(draw_buffers.size(), draw_buffers.data());

    glViewport(0, 0, size.x, size.y);
}

void framebuffer::resize(glm::ivec2 new_size) {
    size = new_size;
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    for(int i = 0; i < tex_params.size(); i++) {
        fb_tex_params& p = tex_params[i];
        texture& t = textures[i];
        t.bind();
        t.delete_texture();
        
        texture_desc desc = get_texture_desc(p.format);

        GLenum fil;
        if(filter == axiom::texture_filter::NEAREST) fil = GL_NEAREST;
        else if(filter == axiom::texture_filter::LINEAR) fil = GL_LINEAR;

        glGenTextures(1, &t.id);
        glBindTexture(t.type, t.id);
        if(t.type == GL_TEXTURE_2D_ARRAY) glTexStorage3D(t.type, 1, desc.formatbits, size.x, size.y, t.size.z);
        else glTexStorage2D(t.type, 1, desc.formatbits, size.x, size.y);
        glTexParameteri(t.type, GL_TEXTURE_MAG_FILTER, fil);
        glTexParameteri(t.type, GL_TEXTURE_MIN_FILTER, fil);
        glTexParameteri(t.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(t.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, get_texture_attachment(p.attachment), GL_TEXTURE_2D, t.id, 0);

        t.size = glm::ivec3(size, 1);
    }
}

framebuffer::~framebuffer() {
    if(initialized) {
        glDeleteFramebuffers(1, &id);
    }
}

void framebuffer::clear(vec4 color, float depth) {
    bind();

    glClearColor(color.x, color.y, color.z, color.q);
    glClearDepthf(depth);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void framebuffer::bind_texture(std::shared_ptr<texture> texture, uint attachment, int32_t binding) {
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->id, 0);

    std::vector<uint> new_draw_buffers;
    bool inserted = false;
    for(int i = 0; i < draw_buffers.size(); ++i) {
        if(!inserted) {
            if(i == binding) {
                inserted = true;
                new_draw_buffers.push_back(attachment);
            } else {
                new_draw_buffers.push_back(draw_buffers[i]);
            }
        } else {
            new_draw_buffers.push_back(draw_buffers[i]);
        }
    }

    if(!inserted) {
        int buffers_size = draw_buffers.size();
        for(int i = 0; i < binding - buffers_size + 1; ++i) {
            new_draw_buffers.push_back(GL_NONE);
        }

        new_draw_buffers[binding] = attachment;
    }

    draw_buffers = new_draw_buffers;
}

texture_desc get_texture_desc(texture_format f) {
    switch(f) {
        case texture_format::R8: {
            return {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
        }
        case texture_format::RG8: {
            return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
        }
        case texture_format::RGB8: {
            return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
        }
        case texture_format::RGBA8: {
            return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
        }

        case texture_format::RF: {
            return {GL_R32F, GL_RED, GL_FLOAT};
        }
        case texture_format::RGF: {
            return {GL_RG32F, GL_RG, GL_FLOAT};
        }
        case texture_format::RGBF: {
            return {GL_RGB32F, GL_RGB, GL_FLOAT};
        }
        case texture_format::RGBAF: {
            return {GL_RGBA32F, GL_RGBA, GL_FLOAT};
        }

        case texture_format::DEPTH16: {
            return {GL_DEPTH_COMPONENT16, GL_RED, GL_UNSIGNED_SHORT};
        }
        case texture_format::DEPTH24: {
            return {GL_DEPTH_COMPONENT24, GL_RED, GL_UNSIGNED_INT};
        }
        case texture_format::DEPTH32: {
            throw std::runtime_error("get_texture_desc: this texture_format enum is not supported in WebGL");
            return {0xFFFFFFFF, GL_RED, GL_FLOAT};
        }
        case texture_format::DEPTHF: {
            return {GL_DEPTH_COMPONENT32F, GL_RED, GL_FLOAT};
        }
    }
}

GLenum get_texture_attachment(texture_attachment attachment) {
    if(attachment >= texture_attachment::COLOR0 && attachment <= texture_attachment::COLOR31) {
        uint i = (uint)attachment - (uint)texture_attachment::COLOR0;

        return GL_COLOR_ATTACHMENT0 + i;
    } else {
        switch(attachment) {
            case texture_attachment::DEPTH:
                return GL_DEPTH_ATTACHMENT;
            case texture_attachment::STENCIL:
                return GL_STENCIL_ATTACHMENT;
            case texture_attachment::DEPTH_STENCIL:
                return GL_DEPTH_STENCIL_ATTACHMENT;
        }
    }
}

}

/*
void get_image(void* ptr, unsigned int buf_size, texture& texture) {
    texture.bind();
    glGetnTexImage(GL_TEXTURE_3D, 0, texture.desc.format, texture.desc.bits, buf_size, ptr);
}
*/

#include <wrapper/webgl/uniform.hpp>