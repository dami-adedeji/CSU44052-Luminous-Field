#ifndef _LIGHT_H_
#define _LIGHT_H_
#include <glm/glm.hpp>

enum class LightType {Directional = 0, Point = 1, Spot = 2};
struct Light
{
    int type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 colour;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outerCutoff;
};

#endif