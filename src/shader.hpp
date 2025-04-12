#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef SHADER_H
#define SHADER_H

class shader
{
public:
    // to initalize the shader
    shader(const std::string &vert_path, const std::string &frag_path);

    // to clean up the shader
    ~shader();

    // to call use shader
    void use();

    // to set mat4 uniforms
    void set_mat4(const glm::mat4 &var, const std::string &loc) const;

    // to set vec3 uniforms
    void set_vec3(const glm::vec3 &var, const std::string &loc) const;

    // to set int uniforms
    void set_int(const int var, const std::string &loc) const;

private:

    // shader program id
    unsigned int program{};

    // to read a shader file into a std::string
    std::string readfile(const std::string &filename);
};

#endif //SHADER_H