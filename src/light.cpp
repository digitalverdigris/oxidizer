#include "light.hpp"

light::light(shader &a_shader, int r_width, int r_height, glm::vec3 a_pos, glm::vec3 a_color)
    : l_shader(a_shader), render_width(r_width), render_height(r_height), pos(a_pos), color(a_color)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // bind vertex buffer and vertex array
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
}

void light::draw(glm::vec3 &view_pos)
{
    l_shader.use();
    l_shader.set_vec3("light_color", color);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    model = glm::scale(model, glm::vec3(16.0f));
    model = glm::translate(model, pos);

    view = glm::lookAt(view_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // projection = glm::perspective(glm::radians(45.0f), (float)render_width / (float)render_height, 0.1f, 100.0f);
    projection = glm::ortho(-(float)render_width / 2, (float)render_width / 2, -(float)render_height / 2, (float)render_height / 2, -100.0f, 100.0f);

    l_shader.set_mat4("model", model);
    l_shader.set_mat4("view", view);
    l_shader.set_mat4("projection", projection);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}