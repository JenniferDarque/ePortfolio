#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"
#include "LightingManager.h"
#include "ViewManager.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include <unordered_map>

class SceneManager
{
public:
    SceneManager(ShaderManager* pShaderManager);
    ~SceneManager();

    void PrepareScene();
    void RenderScene();

    void ProcessInput(GLFWwindow* window);
    void ProcessMouseMovement(double xpos, double ypos);
    void ProcessMouseScroll(double yoffset);

    void SetWindowSize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;
    }

    void PrepareSceneView();
    ViewManager* GetViewManager() const { return m_pViewManager; }

private:
    ShaderManager* m_pShaderManager;
    ShapeMeshes* m_basicMeshes;
    LightingManager* m_pLightingManager;
    ViewManager* m_pViewManager;

    // Camera
    glm::vec3 m_cameraPos = glm::vec3(0.0f, 1.0f, 10.0f);
    glm::vec3 m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_cameraSpeed = 0.05f;
    float m_cameraZoom = 45.0f;

    enum ProjectionMode { PERSPECTIVE, ORTHOGRAPHIC };
    ProjectionMode m_projectionMode = PERSPECTIVE;

    int m_windowWidth = 800;
    int m_windowHeight = 600;

    // Texture storage
    std::unordered_map<std::string, GLuint> m_textureMap;

    // Materials
    struct OBJECT_MATERIAL
    {
        float ambientStrength;
        glm::vec3 ambientColor;
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float shininess;
        std::string tag;
        float uvScaleU = 1.0f;
        float uvScaleV = 1.0f;
    };

    std::vector<OBJECT_MATERIAL> m_objectMaterials;

    void InitMaterials();
    void SetTransformations(glm::vec3 scaleXYZ,
        float Xrot, float Yrot, float Zrot,
        glm::vec3 positionXYZ);
    void SetShaderColor(float r, float g, float b, float a);
    void SetShaderTexture(std::string tag);
    void SetTextureUVScale(float u, float v);
    void SetShaderMaterial(std::string tag);
    bool CreateGLTexture(const char* filename, std::string tag);
};
