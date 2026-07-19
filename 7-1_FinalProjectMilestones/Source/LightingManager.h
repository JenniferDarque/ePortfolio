#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

enum LIGHT_TYPE {
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1
};

struct Light {
    LIGHT_TYPE type;

    // Directional
    glm::vec3 direction;

    // Point
    glm::vec3 position;

    // Shared
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float intensity;

    // Attenuation (point lights only)
    float constant;
    float linear;
    float quadratic;

    bool active;
    std::string tag;
};

class ShaderManager;

class LightingManager {
public:
    std::vector<Light> lights;

    LightingManager();

    // Add a new light to the system
    void AddLight(const Light& light);

    // Send all active lights to the shader
    void Apply(ShaderManager* shader);
};
