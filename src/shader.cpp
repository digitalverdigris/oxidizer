#include "shader.hpp"

shader::shader(const std::string &vert_path, const std::string &frag_path)
{
    // read vertex shader data from source path
    std::string vs_source{readfile(vert_path)};

    // read fragment shader data from source path
    std::string fs_source{readfile(frag_path)};

    // convert vertex shader file path from std::string to const char
    const char *vs_s{vs_source.c_str()};

    // convert fragment shader file path from std::string to const char
    const char *fs_s{fs_source.c_str()};

    // create the vertex shader
    unsigned int vs{glCreateShader(GL_VERTEX_SHADER)};

    // denote the vertex shader source code
    glShaderSource(vs, 1, &vs_s, nullptr);

    // compile the vertex shader source code
    glCompileShader(vs);

    // create the fragment shader
    unsigned int fs{glCreateShader(GL_FRAGMENT_SHADER)};

    // denote the fragment shader source code
    glShaderSource(fs, 1, &fs_s, nullptr);

    // compile the fragment shader source code
    glCompileShader(fs);

    // create shader program
    program = glCreateProgram();

    // attach the vertex shader
    glAttachShader(program, vs);

    // attach the fragment shader
    glAttachShader(program, fs);

    // delete the vertex and fragment shader as they are now attached to the shader program
    glDeleteShader(vs);
    glDeleteShader(fs);

    // link the shader program
    glLinkProgram(program);

    // validate the shader program
    glValidateProgram(program);
}

shader::~shader()
{
    // delete the shader program on cleanup
    glDeleteProgram(program);
}

void shader::use()
{
    // use the shader program
    glUseProgram(program);
}

void shader::set_mat4(const std::string &loc, const glm::mat4 &var) const
{
    // set a mat4 uniform
    glUniformMatrix4fv(glGetUniformLocation(program, loc.c_str()), 1, GL_FALSE, &var[0][0]);
}

void shader::set_vec3(const std::string &loc, const glm::vec3 &var) const
{
    // set a vec3 uniform
    glUniform3fv(glGetUniformLocation(program, loc.c_str()), 1, &var[0]);
}

void shader::set_int(const std::string &loc, const int) const
{
    // set an int uniform
    glUniform1i(glGetUniformLocation(program, loc.c_str()), 0);
}

// shader file reading function
std::string shader::readfile(const std::string &filename)
{
    // create an ifstream to read file into
    std::ifstream file{};

    // open the file
    file.open(filename);

    // ensure file can be opened
    if (file.fail())
    {
        std::cout << "failed to open file" << std::endl;
    }

    // create a string stream buffer
    std::stringstream buffer{};

    // read the file into the buffer
    buffer << file.rdbuf();

    // create a string for the buffer contents
    std::string file_contents{};

    // convert the buffer into a string
    file_contents = buffer.str();

    // close the file
    file.close();

    // return the file contents
    return file_contents;
}