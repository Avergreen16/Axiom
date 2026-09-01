#include <graphicsh.hpp>

#include <core/assets/assets.hpp>
#include <render/wrapper.hpp>

#include "stb_image.h"
#include "stb_image_write.h"


namespace axiom {

void set_blend() {
    
}

void vertices::init() {
    glGenBuffers(1, &vertex_buffer);
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    initialized = true;
}   

void vertices::vertex_buffer_data(void* ptr, uint32_t num_vertices_, uint32_t vertex_size, uint32_t usage) {
    bind();
    num_vertices = num_vertices_;
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * vertex_size, ptr, usage);
}

void vertices::index_buffer_data(void* ptr, uint32_t num_indices_, uint index_type_, uint32_t index_size, uint32_t usage) {
    bind();
    num_indices = num_indices_;
    index_type = index_type_;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * index_size, ptr, usage);
}

void vertices::add_vertex_attribute(uint32_t index, uint32_t size, uint type, uint normalized, uint32_t stride, uint32_t offset) {
    glBindVertexArray(vertex_array);
    if(type == GL_INT || type == GL_UNSIGNED_INT) glVertexAttribIPointer(index, size, type, stride, (void*)offset);
    else glVertexAttribPointer(index, size, type, normalized, stride, (void*)offset);
    glEnableVertexAttribArray(index);  
}

void vertices::bind() {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindVertexArray(vertex_array);
}

void vertices::draw_vertices(uint mode) {
    bind();
    glDrawArrays(mode, 0, num_vertices);
}

void vertices::draw_indices(uint mode) {
    bind();
    glDrawElements(mode, num_indices, index_type, (void*)0);
}

vertices::vertices(vertices&& a) {
    vertex_buffer = a.vertex_buffer;
    a.vertex_buffer = 0;

    vertex_array = a.vertex_array;
    a.vertex_array = 0;

    index_buffer = a.index_buffer;
    a.index_buffer = 0;

    num_indices = a.num_indices;
    a.num_indices = 0;

    num_vertices = a.num_vertices;
    a.num_vertices = 0;

    initialized = a.initialized;
    a.initialized = false;

    index_type = a.index_type;
}

vertices& vertices::operator=(vertices&& a) {
    vertex_buffer = a.vertex_buffer;
    a.vertex_buffer = 0;

    vertex_array = a.vertex_array;
    a.vertex_array = 0;

    index_buffer = a.index_buffer;
    a.index_buffer = 0;

    num_indices = a.num_indices;
    a.num_indices = 0;

    num_vertices = a.num_vertices;
    a.num_vertices = 0;

    initialized = a.initialized;
    a.initialized = false;

    index_type = a.index_type;

    return *this;
}

vertices::~vertices() {
    if(initialized) {
        glDeleteBuffers(1, &vertex_buffer);
        glDeleteBuffers(1, &index_buffer);
        glDeleteVertexArrays(1, &vertex_array);
    }
}

void storage_buffer::init() {
    if(!initialized) {
        glGenBuffers(1, &id);
        initialized = true;
    }
}

void storage_buffer::buffer_data(void* data, uint32_t size_bytes, uint usage) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size_bytes, data, usage);
}

void storage_buffer::buffer_subdata(void* data, uint32_t size_bytes, uint32_t offset) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size_bytes, data);
}

void storage_buffer::bind(uint32_t binding) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
}

void storage_buffer::delete_buffer() {
    glDeleteBuffers(1, &id);
}

storage_buffer::~storage_buffer() {
    delete_buffer();
}

void uniform_buffer::init() {
    if(!initialized) {
        glGenBuffers(1, &id);
        initialized = true;
    }
}

void uniform_buffer::buffer_data(void* data, uint32_t size_bytes, uint usage) {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferData(GL_UNIFORM_BUFFER, size_bytes, data, usage);
}

void uniform_buffer::buffer_subdata(void* data, uint32_t size_bytes, uint32_t offset) {
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size_bytes, data);
}

void uniform_buffer::bind(uint32_t binding) {
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

shader::shader(text_asset vertex_shader, text_asset fragment_shader) {
    compile(vertex_shader, fragment_shader);
}

shader::shader(text_asset vertex_shader, text_asset geometry_shader, text_asset fragment_shader) {
    compile(vertex_shader, geometry_shader, fragment_shader);
}

shader::shader(text_asset vertex_shader, text_asset tess_ctrl_shader, text_asset tess_eval_shader, text_asset geometry_shader, text_asset fragment_shader) {
    compile(vertex_shader, tess_ctrl_shader, tess_eval_shader, geometry_shader, fragment_shader);
}

shader::shader(text_asset compute_shader) {
    compile(compute_shader);
}

shader::shader(shader&& s) {
    id = s.id;
    s.id = 0;
}

shader& shader::operator=(shader&& s) {
    id = s.id;
    s.id = 0;

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

    glGetShaderiv(id, GL_LINK_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(id, log_size, &log_size, &error_log[0]);
        std::cout << "ERROR: Program failed to link:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << fragment_shader.filepath << "\n";
        //std::cout << "Filepath: " << vertex_shader.filepath << "\n";

        glDeleteShader(id);
        return false;
    }

    // delete vertex/fragment shaders after compilation
    glDeleteShader(vs);
    glDeleteShader(fs);

    return true;
}

bool shader::compile(text_asset vertex_shader, text_asset geometry_shader, text_asset fragment_shader) {
    const char* v_ptr = vertex_shader.data.data();
    const char* f_ptr = fragment_shader.data.data();
    const char* g_ptr = geometry_shader.data.data();

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
        std::cout << "ERROR: Vertex shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << vspath << "\n";

        return false;
    }

    uint gs = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(gs, 1, &g_ptr, 0);
    glCompileShader(gs);
    
    // check if geometry shader compiled correctly
    glGetShaderiv(gs, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(gs, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(gs, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << geometry_shader.filepath << "\n";
        std::cout << "ERROR: Geometry shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << gspath << "\n";

        glDeleteShader(gs);
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
        //std::cout << "Filepath: " << fspath << "\n";

        glDeleteShader(vs);
        return false;
    }
    
    // attach and link vertex/fragment shaders
    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, gs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    // delete vertex/fragment shaders after compilation
    glDeleteShader(vs);
    glDeleteShader(gs);
    glDeleteShader(fs);

    return true;
}

bool shader::compile(text_asset vertex_shader, text_asset tess_ctrl_shader, text_asset tess_eval_shader, text_asset geometry_shader, text_asset fragment_shader) {
    const char* v_ptr = vertex_shader.data.data();
    const char* tc_ptr = tess_ctrl_shader.data.data();
    const char* te_ptr = tess_eval_shader.data.data();
    const char* g_ptr = geometry_shader.data.data();
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
        //std::cout << "Filepath: " << vspath << "\n";

        return false;
    }

    uint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
    glShaderSource(tcs, 1, &tc_ptr, 0);
    glCompileShader(tcs);
    
    // check if tess ctrl shader compiled correctly
    glGetShaderiv(tcs, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(tcs, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(tcs, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << tess_ctrl_shader.filepath << "\n";
        std::cout << "ERROR: Tesselation control shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << tcspath << "\n";

        glDeleteShader(tcs);
        return false;
    }
    
    uint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
    glShaderSource(tes, 1, &te_ptr, 0);
    glCompileShader(tes);
    
    // check if tess  shader compiled correctly
    glGetShaderiv(tes, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(tes, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(tes, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << tess_eval_shader.filepath << "\n";
        std::cout << "ERROR: Tesselation evaluation shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << tespath << "\n";

        glDeleteShader(tes);
        return false;
    }

    
    uint gs = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(gs, 1, &g_ptr, 0);
    glCompileShader(gs);
    
    // check if geometry shader compiled correctly
    glGetShaderiv(gs, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(gs, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(gs, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << geometry_shader.filepath << "\n";
        std::cout << "ERROR: Geometry shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << gspath << "\n";

        glDeleteShader(gs);
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
        //std::cout << "Filepath: " << fspath << "\n";

        glDeleteShader(vs);
        return false;
    }
    
    // attach and link vertex/fragment shaders
    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, tcs);
    glAttachShader(id, tes);
    glAttachShader(id, gs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    // delete vertex/fragment shaders after compilation
    glDeleteShader(vs);
    glDeleteShader(tcs);
    glDeleteShader(tes);
    glDeleteShader(gs);
    glDeleteShader(fs);

    return true;
}


bool shader::compile(text_asset compute_shader) {

    const char* c_ptr = compute_shader.data.data();
    uint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &c_ptr, 0);
    glCompileShader(cs);
    
    // check if vertex shader compiled correctly
    int compile_check;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(cs, log_size, &log_size, &error_log[0]);
        std::cout << "error in file: " << compute_shader.filepath << "\n";
        std::cout << "ERROR: Compute shader failed to compile:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << cspath << "\n";

        return false;
    }
    
    // attach and link vertex/fragment shaders
    id = glCreateProgram();
    glAttachShader(id, cs);
    glLinkProgram(id);

    glGetShaderiv(id, GL_LINK_STATUS, &compile_check);
    if(compile_check == GL_FALSE) {
        int log_size = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_size);

        std::vector<char> error_log(log_size);
        glGetShaderInfoLog(id, log_size, &log_size, &error_log[0]);
        std::cout << "ERROR: Program failed to link:\n" << error_log.data() << "\n";
        //std::cout << "Filepath: " << cspath << "\n";

        glDeleteShader(id);
        return false;
    }

    // delete vertex/fragment shaders after compilation
    glDeleteShader(cs);

    return true;
}

void shader::use() {
    glUseProgram(id);
}

void shader::dispatch_compute(glm::uvec3 work_groups) {
    glDispatchCompute(work_groups.x, work_groups.y, work_groups.z);
}

texture::texture(texture_asset& asset, texture_format format, int mip_levels) {
    load(asset, format, mip_levels);
}

void texture::load(texture_asset& asset, texture_format format, int mip_levels) {
    type = GL_TEXTURE_2D;
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

    if(mip_levels > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, mip_levels);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
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

void texture::bind_image(unsigned int binding, unsigned int level, uint format, unsigned int layer) {
    glBindImageTexture(binding, id, level, GL_TRUE, layer, GL_READ_WRITE, format);
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

framebuffer::framebuffer(glm::ivec2 size_, std::vector<fb_tex_params> tp, uint filter) {
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

        if(p.layers > 1) {

            glGenTextures(1, &t.id);
            glBindTexture(GL_TEXTURE_2D_ARRAY, t.id);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, desc.formatbits, size.x, size.y, p.layers);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            t.size = glm::ivec3(size, p.layers);
            t.format = p.format;
            t.type = GL_TEXTURE_2D_ARRAY;
        } else {
            glGenTextures(1, &t.id);
            glBindTexture(GL_TEXTURE_2D, t.id);
            glTexStorage2D(GL_TEXTURE_2D, 1, desc.formatbits, size.x, size.y);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            t.size = glm::ivec3(size, 1);
            t.format = p.format;
            t.type = GL_TEXTURE_2D;
        }

        glFramebufferTexture(GL_FRAMEBUFFER, get_texture_attachment(p.attachment), t.id, 0);

        bool insert = true;
        if(p.attachment == texture_attachment::DEPTH || p.attachment == texture_attachment::STENCIL || p.attachment == texture_attachment::DEPTH_STENCIL) insert = false;

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

    draw_buffers = a.draw_buffers;
}

framebuffer& framebuffer::operator=(framebuffer&& a) noexcept {
    id = a.id;
    a.id = 0;
    textures = std::move(a.textures);
    tex_params = std::move(a.tex_params);
    size = a.size;

    draw_buffers = a.draw_buffers;

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

        glGenTextures(1, &t.id);
        glBindTexture(t.type, t.id);
        if(t.type == GL_TEXTURE_2D_ARRAY) glTexStorage3D(t.type, 1, desc.formatbits, size.x, size.y, t.size.z);
        else glTexStorage2D(t.type, 1, desc.formatbits, size.x, size.y);
        glTexParameteri(t.type, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(t.type, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(t.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(t.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture(GL_FRAMEBUFFER, get_texture_attachment(p.attachment), t.id, 0);

        t.size = glm::ivec3(size, 1);
    }
}

framebuffer::~framebuffer() {
    if(initialized) {
        glDeleteFramebuffers(1, &id);
    }
}

void framebuffer::clear() {
    for(int i = 0; i < textures.size(); ++i) {
        texture& t = textures[i];
        fb_tex_params& p = tex_params[i];
    }
}

void framebuffer::bind_texture(std::shared_ptr<texture> texture, uint attachment, int32_t binding) {
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture->id, 0);

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

texture_asset texture::retrieve() {
    std::vector<uint8_t> vector(size.x * size.y * size.z * num_channels);
    bind();
    
    texture_desc desc = get_texture_desc(format);

    glGetTexImage(type, 0, desc.format, desc.bits, vector.data());

    texture_asset asset;
    asset.data = vector;
    asset.size = {size.x, size.y};
    asset.num_channels = num_channels;

    return asset;
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

        case texture_format::R16: {
            return {GL_R16, GL_RED, GL_UNSIGNED_SHORT};
        }
        case texture_format::RG16: {
            return {GL_RG16, GL_RG, GL_UNSIGNED_SHORT};
        }
        case texture_format::RGB16: {
            return {GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT};
        }
        case texture_format::RGBA16: {
            return {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT};
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
        case texture_format::DEPTH32: {
            return {GL_DEPTH_COMPONENT32, GL_RED, GL_UNSIGNED_INT};
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