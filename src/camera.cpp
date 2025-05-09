#include "camera.hpp"

camera::camera()
{
    proj = glm::ortho(-(float)render_width / 2, (float)render_width / 2, -(float)render_height / 2, (float)render_height / 2, -100.0f, 100.0f);
}
