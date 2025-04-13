#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"

#ifndef CUBE_H
#define CUBE_H

class cube
{
public:
    cube(shader &a_shader, int r_width, int r_height, glm::vec3 a_pos);
    void draw();

private:
    shader &c_shader;

    int render_width, render_height;

    unsigned int VAO{}, VBO{};

    glm::vec3 pos{};

    const float vertices[216]
    {
        // Back face (Red)
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,

        // Front face (Green)
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,

        // Left face (Blue)
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,

        // Right face (Yellow)
        0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
        0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
        0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,

        // Bottom face (Magenta)
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,

        // Top face (Cyan)
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
         0.5f, 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f
    };
};
#endif //CUBE_h