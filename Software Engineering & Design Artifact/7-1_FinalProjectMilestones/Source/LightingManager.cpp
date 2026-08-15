#include "LightingManager.h"
#include "ShaderManager.h"

#define TOTAL_POINT_LIGHTS 5


LightingManager::LightingManager()
{
    //
    // DIRECTIONAL LIGHT (SUN)
    //
    Light sun;
    sun.type = DIRECTIONAL_LIGHT;
    sun.direction = glm::vec3(0.0f, -1.0f, 0.0f);

    sun.ambient = glm::vec3(0.30f, 0.30f, 0.30f);
    sun.diffuse = glm::vec3(0.01f, 0.01f, 0.01f);
    sun.specular = glm::vec3(0.01f, 0.01f, 0.01f);

    sun.intensity = 1.0f;
    sun.active = true;
    sun.tag = "sun";

    lights.push_back(sun);

    //
// POINT LIGHT 0 – LEFT ROUND POOL (BLUE)
//
    Light p0;
    p0.type = POINT_LIGHT;
    p0.position = glm::vec3(-11.0f, 0.6f, 0.0f);   // raised for wider spread

    p0.ambient = glm::vec3(0.05f, 0.05f, 0.10f);
    p0.diffuse = glm::vec3(1.8f, 1.8f, 3.6f);     // stronger blue glow
    p0.specular = glm::vec3(2.0f, 2.0f, 4.0f);     // stronger specular

    p0.intensity = 1.0f;
    p0.constant = 1.0f;
    p0.linear = 0.045f;                         // slower falloff
    p0.quadratic = 0.0075f;                        // much larger radius

    p0.active = true;
    p0.tag = "poolLeftBlue";

    lights.push_back(p0);

    //
    // POINT LIGHT 1 – RIGHT ROUND POOL (BLUE)
    //
    Light p1 = p0;
    p1.position = glm::vec3(11.0f, 0.6f, 0.0f);     // raised for symmetry
    p1.tag = "poolRightBlue";
    lights.push_back(p1);

    //
    // POINT LIGHT 2 – LEFT RECTANGULAR POOL (GREEN)
    //
    Light p2;
    p2.type = POINT_LIGHT;
    p2.position = glm::vec3(-7.5f, 0.6f, 0.0f);     // raised for wider spread

    p2.ambient = glm::vec3(0.05f, 0.10f, 0.05f);
    p2.diffuse = glm::vec3(1.6f, 4.0f, 1.6f);      // stronger green glow
    p2.specular = glm::vec3(2.0f, 5.0f, 2.0f);      // stronger specular

    p2.intensity = 1.0f;
    p2.constant = 1.0f;
    p2.linear = 0.045f;                         // slower falloff
    p2.quadratic = 0.0075f;                        // much larger radius

    p2.active = true;
    p2.tag = "poolLeftGreen";

    lights.push_back(p2);

    //
    // POINT LIGHT 3 – RIGHT RECTANGULAR POOL (GREEN)
    //
    Light p3 = p2;
    p3.position = glm::vec3(7.5f, 0.6f, 0.0f);      // raised for symmetry
    p3.tag = "poolRightGreen";
    lights.push_back(p3);


    //
    // POINT LIGHT 4 – LEFT LAMPPOST (RED)
    //
    Light p4;
    p4.type = POINT_LIGHT;
    p4.position = glm::vec3(-3.5f, 3.5f, 0.0f);

    p4.ambient = glm::vec3(0.25f, 0.08f, 0.08f);
    p4.diffuse = glm::vec3(2.2f, 0.25f, 0.25f);
    p4.specular = glm::vec3(2.8f, 0.35f, 0.35f);

    p4.intensity = 1.0f;
    p4.constant = 1.0f;
    p4.linear = 0.09f;
    p4.quadratic = 0.032f;

    p4.active = true;
    p4.tag = "lampLeftRed";

    lights.push_back(p4);

    //
    // POINT LIGHT 5 – RIGHT LAMPPOST (RED)
    //
    Light p5 = p4;
    p5.position = glm::vec3(3.5f, 3.5f, 0.0f);
    p5.tag = "lampRightRed";
    lights.push_back(p5);
}

void LightingManager::AddLight(const Light& light)
{
    lights.push_back(light);
}

void LightingManager::Apply(ShaderManager* shader)
{
    //
    // Enable lighting in the shader
    //
    shader->setBoolValue("bUseLighting", true);

    //
    // Directional light: find the first DIRECTIONAL_LIGHT
    //
    for (const Light& L : lights)
    {
        if (L.type == DIRECTIONAL_LIGHT)
        {
            shader->setVec3Value("directionalLight.direction", L.direction);
            shader->setVec3Value("directionalLight.ambient", L.ambient);
            shader->setVec3Value("directionalLight.diffuse", L.diffuse);
            shader->setVec3Value("directionalLight.specular", L.specular);
            shader->setBoolValue("directionalLight.bActive", L.active);
            break;
        }
    }

    //
    // Point lights: map up to TOTAL_POINT_LIGHTS = 5
    //
    int pointIndex = 0;
    for (const Light& L : lights)
    {
        if (L.type != POINT_LIGHT)
            continue;

        if (pointIndex >= TOTAL_POINT_LIGHTS)
            break;

        std::string base = "pointLights[" + std::to_string(pointIndex) + "]";

        shader->setVec3Value(base + ".position", L.position);
        shader->setVec3Value(base + ".ambient", L.ambient);
        shader->setVec3Value(base + ".diffuse", L.diffuse);
        shader->setVec3Value(base + ".specular", L.specular);
        shader->setBoolValue(base + ".bActive", L.active);

        pointIndex++;
    }

    //
    // Any unused point light slots are disabled
    //
    for (int i = pointIndex; i < TOTAL_POINT_LIGHTS; i++)
    {
        std::string base = "pointLights[" + std::to_string(i) + "]";
        shader->setBoolValue(base + ".bActive", false);
    }

    //
    // Spotlight not used: ensure it's off
    //
    shader->setBoolValue("spotLight.bActive", false);
}
