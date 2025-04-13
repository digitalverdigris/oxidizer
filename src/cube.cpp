#include "cube.hpp"

cube::cube(shader &a_shader, int r_width, int r_height, glm::vec3 a_pos)
    : c_shader(a_shader), render_width(r_width), render_height(r_height), pos(a_pos)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // bind vertex buffer and vertex array
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
}

void cube::draw()
{
    c_shader.use();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
    view = glm::translate(view, pos);
    projection = glm::perspective(glm::radians(45.0f), (float)render_width / (float)render_height, 0.1f, 100.0f);

    c_shader.set_mat4("model", model);
    c_shader.set_mat4("view", view);
    c_shader.set_mat4("projection", projection);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}