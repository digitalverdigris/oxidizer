#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define WIDTH 640
#define HEIGHT 480

//shader paths
const char *VERTEX_SHADER_PATH = "shaders/vert.glsl";
const char *FRAGMENT_SHADER_PATH = "shaders/frag.glsl";

unsigned int load_shader(const char *vertex_path, const char *fragment_path)
{
    unsigned int ID;
    std::string vertex_code, fragment_code;

    std::ifstream v_shader_file, f_shader_file;

    v_shader_file.open(vertex_path);
    f_shader_file.open(fragment_path);

    std::stringstream v_shader_stream, f_shader_stream;

    v_shader_stream << v_shader_file.rdbuf();
    f_shader_stream << f_shader_file.rdbuf();

    v_shader_file.close();
    f_shader_file.close();

    vertex_code = v_shader_stream.str();
    fragment_code = f_shader_stream.str();

    const char* v_shader_code = vertex_code.c_str();
    const char *f_shader_code = fragment_code.c_str();

    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &v_shader_code, NULL);
    glCompileShader(vertex);
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &f_shader_code, NULL);
    glCompileShader(fragment);
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return ID;
}

// key callback script
void key_callback(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// window resize script
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, WIDTH, HEIGHT);
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
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    std::cout << "window hints passed!" << std::endl;

    // creating a window
    std::cout << "creating a GLFW window..." << std::endl;
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "verdigris engine", NULL, NULL);
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
    std::cout << "GLAD initialized";

    unsigned int shader_id = load_shader(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);

    // triangle vertices
    float vertices[9] =
    {
        -0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    // create vertex array and vertex buffer array
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // bind vertex buffer and vertex array
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // process inputs
        key_callback(window);

        // clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw triangle
        glUseProgram(shader_id);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader_id);

    // terminate GLFW
    std::cout << "terminating GLFW..." << std::endl;
    glfwTerminate();
    std::cout << "GLFW terminated!" << std::endl;
}
