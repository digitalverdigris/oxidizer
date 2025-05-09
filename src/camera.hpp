#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class camera
{
public:
    camera();
    glm::mat4 &get_view() {return view;};
    glm::mat4 &get_proj() {return proj;};
    glm::vec3 &get_pos() {return pos;};

private:
    void update_front();

    glm::vec3 pos{0.0f, 0.0f, 0.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 world_up{0.0f, 1.0f, 0.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};

    int render_width{};
    int render_height{};

    glm::mat4 proj{1.0f};
    glm::mat4 view{1.0f};
};
#endif // CAMERA_H