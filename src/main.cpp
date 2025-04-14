#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"
#include "cube.hpp"
#include "light.hpp"

#define RENDER_WIDTH 100
#define RENDER_HEIGHT 100

#define SCALE 6

#define SCREEN_WIDTH (RENDER_WIDTH * SCALE)
#define SCREEN_HEIGHT (RENDER_HEIGHT * SCALE)

//shader paths
const char *LIGHT_VERTEX_SHADER_PATH = "shaders/light_vert.glsl";
const char *LIGHT_FRAGMENT_SHADER_PATH = "shaders/light_frag.glsl";

const char *CUBE_VERTEX_SHADER_PATH = "shaders/cube_vert.glsl";
const char *CUBE_FRAGMENT_SHADER_PATH = "shaders/cube_frag.glsl";

const char *FB_VERTEX_SHADER_PATH = "shaders/framebuffer_vert.glsl";
const char *FB_FRAGMENT_SHADER_PATH = "shaders/framebuffer_frag.glsl";


void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// key callback script
void key_callback(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // GLFW initialization
    std::cout << "initializing GLFW..." << std::endl;
    if (!glfwInit())
    {
        std::cout << "GLFW initialization failed!" << std::endl;
        return -1;
    }
    std::cout << "GLFW initialized!" << std::endl;

    // GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    std::cout << "window hints passed!" << std::endl;

    // creating a window
    std::cout << "creating a GLFW window..." << std::endl;
    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "GLFW window created!" << std::endl;
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD initialization (loading openGL function pointers)
    std::cout << "initializing GLAD..." << std::endl;
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "GLAD initialization failed!" << std::endl;
        return -1;
    }
    std::cout << "GLAD initialized" << std::endl;

    glEnable(GL_DEPTH_TEST);

    shader l_program{LIGHT_VERTEX_SHADER_PATH, LIGHT_FRAGMENT_SHADER_PATH};

    shader c_program{CUBE_VERTEX_SHADER_PATH, CUBE_FRAGMENT_SHADER_PATH};

    std::vector<cube> cubes;

    cubes.push_back(cube{c_program, RENDER_WIDTH, RENDER_HEIGHT, glm::vec3( 0.0f,  2.5f, -10.0f), glm::vec3(1.0f, 0.0f, 0.0f)});
    cubes.push_back(cube{c_program, RENDER_WIDTH, RENDER_HEIGHT, glm::vec3(-2.0f, -2.0f, -10.0f), glm::vec3(0.0f, 1.0f, 0.0f)});
    cubes.push_back(cube{c_program, RENDER_WIDTH, RENDER_HEIGHT, glm::vec3( 2.0f, -2.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f)});

    shader fb_program{FB_VERTEX_SHADER_PATH, FB_FRAGMENT_SHADER_PATH};

    float quad_vertices[] = 
    {
        // positions   // tex_coords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    fb_program.use();
    fb_program.set_int("screen_texture", 0);

    unsigned int FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    unsigned int texture_colorbuffer;
    glGenTextures(1, &texture_colorbuffer);
    glBindTexture(GL_TEXTURE_2D, texture_colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_colorbuffer, 0);

    unsigned int RBO;
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, RENDER_WIDTH, RENDER_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // process inputs
        key_callback(window);

        // clear screen
        glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glEnable(GL_DEPTH_TEST);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw cube to framebuffer

        light a_light{l_program, RENDER_WIDTH, RENDER_HEIGHT, glm::vec3(0.0, 0.0f, -10.0f), glm::vec3(1.0f)};

        for (cube a_cube : cubes)
        {
            a_cube.draw(a_light.get_color());
        };

        a_light.draw();

        glBindVertexArray(0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer
        (
            0, 0, RENDER_WIDTH, RENDER_HEIGHT,
            0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );

        // swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // terminate GLFW
    std::cout << "terminating GLFW..." << std::endl;
    glfwTerminate();
    std::cout << "GLFW terminated!" << std::endl;
}

