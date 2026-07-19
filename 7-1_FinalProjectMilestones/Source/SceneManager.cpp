#include "SceneManager.h"
#include "stb_image.h"
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <algorithm>

SceneManager::SceneManager(ShaderManager* pShaderManager)
    : m_pShaderManager(pShaderManager),
    m_basicMeshes(new ShapeMeshes()),
    m_pLightingManager(new LightingManager()),
    m_pViewManager(nullptr)
{
    m_pViewManager = new ViewManager(m_pShaderManager);
}

SceneManager::~SceneManager()
{
    delete m_basicMeshes;
    delete m_pLightingManager;
    delete m_pViewManager;
}

// -------------------- CAMERA VIEW / PROJECTION --------------------
void SceneManager::PrepareSceneView()
{
    glm::mat4 view = glm::lookAt(
        m_cameraPos,
        m_cameraPos + m_cameraFront,
        m_cameraUp
    );

    glm::mat4 projection;

    if (m_projectionMode == PERSPECTIVE)
    {
        projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(m_windowWidth) / m_windowHeight,
            0.1f,
            100.0f
        );
    }
    else // ORTHOGRAPHIC
    {
        float orthoScale = 10.0f; // adjust based on scene size

        projection = glm::ortho(
            -orthoScale,
            orthoScale,
            -orthoScale,
            orthoScale,
            -100.0f,
            100.0f
        );
    }

    m_pShaderManager->use();
    m_pShaderManager->setMat4Value("view", view);
    m_pShaderManager->setMat4Value("projection", projection);
}


// -------------------- INPUT / CAMERA CONTROL --------------------
void SceneManager::ProcessInput(GLFWwindow* window)
{
    // Forward / backward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += m_cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= m_cameraSpeed * m_cameraFront;

    // Calculate right vector once
    glm::vec3 right = glm::normalize(glm::cross(m_cameraFront, m_cameraUp));

    // Strafe left / right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= right * m_cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += right * m_cameraSpeed;

    // Vertical movement
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        m_cameraPos += m_cameraUp * m_cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        m_cameraPos -= m_cameraUp * m_cameraSpeed;

    // Escape quits
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Arrow Up = forward
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        m_cameraPos += m_cameraSpeed * m_cameraFront;

    // Arrow Down = backward
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        m_cameraPos -= m_cameraSpeed * m_cameraFront;

    // Arrow Left = strafe left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        m_cameraPos -= right * m_cameraSpeed;

    // Arrow Right = strafe right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        m_cameraPos += right * m_cameraSpeed;

    // Orthographic view (O key)
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        m_projectionMode = ORTHOGRAPHIC;

    // Perspective view (P key)
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        m_projectionMode = PERSPECTIVE;
}

void SceneManager::ProcessMouseMovement(double xpos, double ypos)
{
    static bool first = true;
    static float lastX = xpos;
    static float lastY = ypos;

    if (first)
    {
        lastX = xpos;
        lastY = ypos;
        first = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    m_cameraFront = glm::normalize(front);
}

void SceneManager::ProcessMouseScroll(double yoffset)
{
    m_cameraSpeed += yoffset * 0.05f;   // adjust sensitivity as needed

    if (m_cameraSpeed < 0.01f)
        m_cameraSpeed = 0.01f;

    if (m_cameraSpeed > 1.0f)
        m_cameraSpeed = 1.0f;
}


// -------------------- TEXTURES --------------------
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0, height = 0, channels = 0;
    GLuint textureID = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    if (!image || width <= 0 || height <= 0)
    {
        if (image) stbi_image_free(image);
        std::cout << "Failed to load texture: " << filename << std::endl;
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_textureMap[tag] = textureID;   // store it

    return true;
}

// -------------------- MATERIALS --------------------
void SceneManager::InitMaterials()
{
    OBJECT_MATERIAL skyMaterial;
    skyMaterial.ambientStrength = 0.2f;
    skyMaterial.ambientColor = glm::vec3(1.0f);
    skyMaterial.diffuseColor = glm::vec3(1.0f);
    skyMaterial.specularColor = glm::vec3(0.0f);
    skyMaterial.shininess = 1.0f;
    skyMaterial.tag = "skyMaterial";
    m_objectMaterials.push_back(skyMaterial);

    OBJECT_MATERIAL grassMaterial;
    grassMaterial.ambientStrength = 0.3f;
    grassMaterial.ambientColor = glm::vec3(0.35f, 0.75f, 0.35f);
    grassMaterial.diffuseColor = glm::vec3(0.35f, 0.75f, 0.35f);
    grassMaterial.specularColor = glm::vec3(0.10f);
    grassMaterial.shininess = 8.0f;
    grassMaterial.uvScaleU = 2.0f;
    grassMaterial.uvScaleV = 2.0f;
    grassMaterial.tag = "grassMaterial";
    m_objectMaterials.push_back(grassMaterial);

    OBJECT_MATERIAL brickMaterial;
    brickMaterial.ambientStrength = 0.2f;
    brickMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.1f);
    brickMaterial.diffuseColor = glm::vec3(0.2f, 0.1f, 0.1f);
    brickMaterial.specularColor = glm::vec3(0.01f);
    brickMaterial.shininess = 2.0f;
    brickMaterial.tag = "brickMaterial";
    m_objectMaterials.push_back(brickMaterial);

    OBJECT_MATERIAL cementMaterial;
    cementMaterial.ambientStrength = 0.3f;
    cementMaterial.ambientColor = glm::vec3(0.60f);
    cementMaterial.diffuseColor = glm::vec3(0.60f);
    cementMaterial.specularColor = glm::vec3(0.12f);
    cementMaterial.shininess = 4.0f;
    cementMaterial.tag = "cementMaterial";
    m_objectMaterials.push_back(cementMaterial);

    OBJECT_MATERIAL darkConcreteMaterial;
    darkConcreteMaterial.ambientStrength = 0.3f;
    darkConcreteMaterial.ambientColor = glm::vec3(0.30f);
    darkConcreteMaterial.diffuseColor = glm::vec3(0.30f);
    darkConcreteMaterial.specularColor = glm::vec3(0.08f);
    darkConcreteMaterial.shininess = 2.0f;
    darkConcreteMaterial.uvScaleU = 4.0f;
    darkConcreteMaterial.uvScaleV = 4.0f;
    darkConcreteMaterial.tag = "darkConcreteMaterial";
    m_objectMaterials.push_back(darkConcreteMaterial);

    OBJECT_MATERIAL bronzeMaterial;
    bronzeMaterial.ambientStrength = 0.3f;
    bronzeMaterial.ambientColor = glm::vec3(0.55f, 0.47f, 0.14f);
    bronzeMaterial.diffuseColor = glm::vec3(0.55f, 0.47f, 0.14f);
    bronzeMaterial.specularColor = glm::vec3(0.45f, 0.45f, 0.35f);
    bronzeMaterial.shininess = 24.0f;
    bronzeMaterial.tag = "bronzeMaterial";
    m_objectMaterials.push_back(bronzeMaterial);

    OBJECT_MATERIAL waterMaterial;
    waterMaterial.ambientStrength = 0.4f;
    waterMaterial.ambientColor = glm::vec3(0.20f, 0.40f, 0.80f);
    waterMaterial.diffuseColor = glm::vec3(0.20f, 0.40f, 0.80f);
    waterMaterial.specularColor = glm::vec3(0.70f, 0.70f, 0.90f);
    waterMaterial.shininess = 82.0f;
    waterMaterial.tag = "waterMaterial";
    m_objectMaterials.push_back(waterMaterial);

    OBJECT_MATERIAL blackmetalMaterial;
    blackmetalMaterial.ambientStrength = 0.3f;
    blackmetalMaterial.ambientColor = glm::vec3(0.05f);
    blackmetalMaterial.diffuseColor = glm::vec3(0.05f);
    blackmetalMaterial.specularColor = glm::vec3(0.20f);
    blackmetalMaterial.shininess = 48.0f;
    blackmetalMaterial.tag = "blackmetalMaterial";
    m_objectMaterials.push_back(blackmetalMaterial);

    OBJECT_MATERIAL glassMaterial;
    glassMaterial.ambientStrength = 0.3f;
    glassMaterial.ambientColor = glm::vec3(0.06f, 0.06f, 0.04f);
    glassMaterial.diffuseColor = glm::vec3(0.06f, 0.06f, 0.04f);
    glassMaterial.specularColor = glm::vec3(0.05f);
    glassMaterial.shininess = 12.0f;
    glassMaterial.tag = "glassMaterial";
    m_objectMaterials.push_back(glassMaterial);

    OBJECT_MATERIAL woodMaterial;
    woodMaterial.ambientStrength = 0.3f;
    woodMaterial.ambientColor = glm::vec3(0.25f, 0.07f, 0.01f);
    woodMaterial.diffuseColor = glm::vec3(0.25f, 0.07f, 0.01f);
    woodMaterial.specularColor = glm::vec3(0.02f);
    woodMaterial.shininess = 2.0f;
    woodMaterial.uvScaleU = 1.5f;
    woodMaterial.uvScaleV = 1.5f;
    woodMaterial.tag = "woodMaterial";
    m_objectMaterials.push_back(woodMaterial);

    OBJECT_MATERIAL roofMaterial;
    roofMaterial.ambientStrength = 0.3f;
    roofMaterial.ambientColor = glm::vec3(0.06f, 0.01f, 0.01f);
    roofMaterial.diffuseColor = glm::vec3(0.06f, 0.01f, 0.01f);
    roofMaterial.specularColor = glm::vec3(0.01f);
    roofMaterial.shininess = 2.0f;
    roofMaterial.uvScaleU = 4.0f;
    roofMaterial.uvScaleV = 4.0f;
    roofMaterial.tag = "roofMaterial";
    m_objectMaterials.push_back(roofMaterial);
}

// -------------------- PREPARE SCENE --------------------
void SceneManager::PrepareScene()
{
    // Load all meshes
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadTaperedCylinderMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadSphereMesh();
    m_basicMeshes->LoadTorusMesh();
    m_basicMeshes->LoadPyramid4Mesh();

    // Load textures (adapt filenames to your actual files)
    CreateGLTexture("grass.jpg", "grass");
    CreateGLTexture("brick.jpg", "brick");
    CreateGLTexture("cement.jpg", "cement");
    CreateGLTexture("bronze.jpg", "bronze");
    CreateGLTexture("cement2.jpg", "cement2");
    CreateGLTexture("glass.jpg", "glass");
    CreateGLTexture("blackmetal.jpg", "blackmetal");
    CreateGLTexture("water.jpg", "water");
    CreateGLTexture("sky.jpg", "sky");
    CreateGLTexture("darkconcrete.jpg", "darkconcrete");
    CreateGLTexture("wood.jpg", "wood");
    CreateGLTexture("roof.jpg", "roof");

    InitMaterials();
}

// -------------------- TRANSFORMS / RENDER --------------------
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float Xrot, float Yrot, float Zrot,
    glm::vec3 positionXYZ)
{
    glm::mat4 model(1.0f);

    model = glm::translate(model, positionXYZ);
    model = glm::rotate(model, glm::radians(Xrot), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(Yrot), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(Zrot), glm::vec3(0, 0, 1));
    model = glm::scale(model, scaleXYZ);

    m_pShaderManager->setMat4Value("model", model);
}

void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
    glm::vec4 color(r, g, b, a);
    m_pShaderManager->setIntValue("bUseTexture", false);
    m_pShaderManager->setVec4Value("objectColor", color);
}

void SceneManager::SetShaderTexture(std::string tag)
{
    int slot = 0; // we’ll just use texture unit 0 for now

    m_pShaderManager->setIntValue("bUseTexture", true);
    m_pShaderManager->setSampler2DValue("objectTexture", slot);

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureMap[tag]);
}


void SceneManager::SetTextureUVScale(float u, float v)
{
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
}

void SceneManager::SetShaderMaterial(std::string tag)
{
    OBJECT_MATERIAL mat;
    bool found = false;
    for (auto& m : m_objectMaterials)
    {
        if (m.tag == tag)
        {
            mat = m;
            found = true;
            break;
        }
    }
    if (!found) return;

    m_pShaderManager->setVec3Value("material.ambientColor", mat.ambientColor);
    m_pShaderManager->setFloatValue("material.ambientStrength", mat.ambientStrength);
    m_pShaderManager->setVec3Value("material.diffuseColor", mat.diffuseColor);
    m_pShaderManager->setVec3Value("material.specularColor", mat.specularColor);
    m_pShaderManager->setFloatValue("material.shininess", mat.shininess);
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(mat.uvScaleU, mat.uvScaleV));
}

void SceneManager::RenderScene()
{
    if (!m_pShaderManager)
        return;

    m_pShaderManager->use();

    // REQUIRED for Blinn–Phong specular
    m_pShaderManager->setVec3Value("viewPosition", m_cameraPos);

    m_pShaderManager->setBoolValue("bUseLighting", true);

    m_pLightingManager->Apply(m_pShaderManager);

    // --- Skybox ---
    glDepthMask(GL_FALSE);

    SetTransformations(
        glm::vec3(100.0f, 100.0f, 100.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f)
    );
    SetShaderTexture("sky");
    SetTextureUVScale(1.0f, 1.0f);
    SetShaderMaterial("skyMaterial");
    m_basicMeshes->DrawBoxMesh();

    glDepthMask(GL_TRUE);

    // --- Grass Plane ---
    SetTransformations(
        glm::vec3(33.0f, 1.0f, 33.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, -0.1f, 0.0f)
    );
    SetShaderTexture("grass");
    SetShaderMaterial("grassMaterial");
    m_basicMeshes->DrawPlaneMesh();

    // --- Brick Box ---
    SetTransformations(
        glm::vec3(5.5f, 1.2f, 5.5f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.95f, 0.0f)
    );
    SetShaderTexture("brick");
    SetShaderMaterial("brickMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Statue Base: Quarter Blocks ---
    glm::vec3 quarterScale(0.75f, 3.0f, 3.0f);

    // Left Cement Quarter
    SetTransformations(
        quarterScale,
        0.0f, 0.0f, 0.0f,
        glm::vec3(-1.125f, 2.7f, 0.0f)
    );
    SetShaderTexture("cement");
    SetShaderMaterial("cementMaterial");
    m_basicMeshes->DrawBoxMesh();

    // Left Dark Concrete Quarter
    SetTransformations(
        quarterScale,
        0.0f, 0.0f, 0.0f,
        glm::vec3(-0.375f, 2.7f, 0.0f)
    );
    SetShaderTexture("darkconcrete");
    SetShaderMaterial("darkConcreteMaterial");
    m_basicMeshes->DrawBoxMesh();

    // Right Cement Quarter
    SetTransformations(
        quarterScale,
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.375f, 2.7f, 0.0f)
    );
    SetShaderTexture("cement");
    SetShaderMaterial("cementMaterial");
    m_basicMeshes->DrawBoxMesh();

    // Right Dark Concrete Quarter
    SetTransformations(
        quarterScale,
        0.0f, 0.0f, 0.0f,
        glm::vec3(1.125f, 2.7f, 0.0f)
    );
    SetShaderTexture("darkconcrete");
    SetShaderMaterial("darkConcreteMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Statue: Central Column ---
    SetTransformations(
        glm::vec3(1.5f, 8.0f, 1.5f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 4.2f, 0.0f)
    );
    SetShaderTexture("cement");
    SetShaderMaterial("cementMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    // --- Statue: Body ---
    SetTransformations(
        glm::vec3(0.75f, 6.0f, 0.75f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 10.95f, 0.0f)
    );
    SetShaderTexture("bronze");
    SetShaderMaterial("bronzeMaterial");
    m_basicMeshes->DrawConeMesh();

    // --- Statue: Head ---
    SetTransformations(
        glm::vec3(0.18f, 0.18f, 0.18f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 16.80f, 0.0f)
    );
    SetShaderTexture("bronze");
    SetShaderMaterial("bronzeMaterial");
    m_basicMeshes->DrawSphereMesh();

    // --- Left Round Pool ---
    SetTransformations(
        glm::vec3(3.0f, 0.2f, 3.0f),
        0.0f, 90.0f, 0.0f,
        glm::vec3(-11.0f, 0.02f, 0.0f)
    );
    SetShaderTexture("water");
    SetShaderMaterial("waterMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    // --- Left Rectangular Pool ---
    SetTransformations(
        glm::vec3(12.0f, 0.2f, 4.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(-4.5f, 0.02f, 0.0f)
    );
    SetShaderTexture("water");
    SetShaderMaterial("waterMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Right Round Pool ---
    SetTransformations(
        glm::vec3(3.0f, 0.2f, 3.0f),
        0.0f, 90.0f, 0.0f,
        glm::vec3(11.0f, 0.02f, 0.0f)
    );
    SetShaderTexture("water");
    SetShaderMaterial("waterMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    // --- Right Rectangular Pool ---
    SetTransformations(
        glm::vec3(12.0f, 0.2f, 4.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(4.5f, 0.02f, 0.0f)
    );
    SetShaderTexture("water");
    SetShaderMaterial("waterMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Round Step ---
    SetTransformations(
        glm::vec3(5.5f, 0.3f, 5.5f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.08f, 0.0f)
    );
    SetShaderTexture("cement");
    SetShaderMaterial("cementMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    // --- Walkways ---
    SetTransformations(
        glm::vec3(2.0f, 0.1f, 16.25f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.08f, -11.25f)
    );
    SetShaderTexture("brick");
    SetShaderMaterial("brickMaterial");
    m_basicMeshes->DrawBoxMesh();

    SetTransformations(
        glm::vec3(2.0f, 0.1f, 16.25f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.08f, 11.25f)
    );
    SetShaderTexture("brick");
    SetShaderMaterial("brickMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Outer Ring (Road) ---
    SetTransformations(
        glm::vec3(22.0f, 0.1f, 26.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, -0.1f, 0.0f)
    );
    SetShaderTexture("cement");
    SetShaderMaterial("cementMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    // --- Inner Grass Ring ---
    SetTransformations(
        glm::vec3(19.5f, 0.1f, 19.5f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f)
    );
    SetShaderTexture("grass");
    SetShaderMaterial("grassMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    // --- Lampposts ---
    SetTransformations(
        glm::vec3(0.1f, 3.0f, 0.1f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(-3.5f, 0.35f, 0.0f)
    );
    SetShaderTexture("blackmetal");
    SetShaderMaterial("blackmetalMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    SetTransformations(
        glm::vec3(0.2f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(-3.5f, 3.5f, 0.0f)
    );
    SetShaderTexture("glass");
    SetShaderMaterial("glassMaterial");
    m_basicMeshes->DrawSphereMesh();

    SetTransformations(
        glm::vec3(0.1f, 3.0f, 0.1f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(3.5f, 0.35f, 0.0f)
    );
    SetShaderTexture("blackmetal");
    SetShaderMaterial("blackmetalMaterial");
    m_basicMeshes->DrawTaperedCylinderMesh();

    SetTransformations(
        glm::vec3(0.2f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(3.5f, 3.5f, 0.0f)
    );
    SetShaderTexture("glass");
    SetShaderMaterial("glassMaterial");
    m_basicMeshes->DrawSphereMesh();

    // --- Building Base ---
    SetTransformations(
        glm::vec3(6.0f, 3.0f, 10.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(-25.0f, 1.5f, 0.0f)
    );
    SetShaderTexture("brick");
    SetShaderMaterial("brickMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Door ---
    SetTransformations(
        glm::vec3(1.5f, 2.0f, 0.1f),
        0.0f, 90.0f, 0.0f,
        glm::vec3(-22.0f, 1.0f, 0.0f)
    );
    SetShaderTexture("wood");
    SetShaderMaterial("woodMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Windows ---
    auto DrawWindow = [&](glm::vec3 pos, float rotY)
        {
            SetTransformations(glm::vec3(1.0f, 1.0f, 0.1f), 0.0f, rotY, 0.0f, pos);
            SetShaderTexture("glass");
            SetShaderMaterial("glassMaterial");
            m_basicMeshes->DrawBoxMesh();
        };

    DrawWindow(glm::vec3(-22.0f, 1.5f, 2.5f), 90.0f);
    DrawWindow(glm::vec3(-22.0f, 1.5f, -2.5f), 90.0f);
    DrawWindow(glm::vec3(-25.0f, 1.5f, -5.1f), 0.0f);
    DrawWindow(glm::vec3(-25.0f, 1.5f, 5.1f), 0.0f);

    for (int i = -2; i <= 2; i += 2)
        DrawWindow(glm::vec3(-28.0f, 1.5f, i * 2.0f), 90.0f);

    // --- Roof ---
    SetTransformations(
        glm::vec3(6.5f, 2.5f, 10.5f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(-25.0f, 4.15f, 0.0f)
    );
    SetShaderTexture("roof");
    SetShaderMaterial("roofMaterial");
    m_basicMeshes->DrawPyramid4Mesh();

    // --- Bronze Doorknob ---
    SetTransformations(
        glm::vec3(0.05f, 0.05f, 0.05f),
        0.0f, 90.0f, 0.0f,
        glm::vec3(-21.5f, 1.0f, 0.8f)
    );
    SetShaderTexture("bronze");
    SetShaderMaterial("bronzeMaterial");
    m_basicMeshes->DrawSphereMesh();

}






/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
/*void SceneManager::RenderScene()

    /****************************************************************/
    /*
  
    

    // --- Building Base (Brick) ---
    SetTransformations(glm::vec3(6.0f, 3.0f, 10.0f), 0, 0, 0, glm::vec3(-25.0f, 1.5f, 0.0f));
    SetShaderTexture("brick");
    SetShaderMaterial("brickMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Door (Wood) ---
    SetTransformations(glm::vec3(1.5f, 2.0f, 0.1f), 0, 90.0f, 0, glm::vec3(-22.0f, 1.0f, 0.0f));
    SetShaderTexture("wood");
    SetShaderMaterial("woodMaterial");
    m_basicMeshes->DrawBoxMesh();

    // --- Windows (Glass) ---
    auto DrawWindow = [&](glm::vec3 pos, float rotY)
        {
            SetTransformations(glm::vec3(1.0f, 1.0f, 0.1f), 0, rotY, 0, pos);
            SetShaderTexture("glass");
            SetShaderMaterial("glassMaterial");
            m_basicMeshes->DrawBoxMesh();
        };

    DrawWindow(glm::vec3(-22.0f, 1.5f, 2.5f), 90.0f);
    DrawWindow(glm::vec3(-22.0f, 1.5f, -2.5f), 90.0f);
    DrawWindow(glm::vec3(-25.0f, 1.5f, -5.1f), 0.0f);
    DrawWindow(glm::vec3(-25.0f, 1.5f, 5.1f), 0.0f);

    for (int i = -2; i <= 2; i += 2)
        DrawWindow(glm::vec3(-28.0f, 1.5f, i * 2.0f), 90.0f);

    // --- Roof ---
    SetTransformations(glm::vec3(6.5f, 2.5f, 10.5f), 0, 0, 0, glm::vec3(-25.0f, 4.15f, 0.0f));
    SetShaderTexture("roof");
    SetShaderMaterial("roofMaterial");
    m_basicMeshes->DrawPyramid4Mesh();

    // --- Bronze Doorknob ---
    SetTransformations(glm::vec3(0.05f, 0.05f, 0.05f), 0, 90.0f, 0, glm::vec3(-21.5f, 1.0f, 0.8f));
    SetShaderTexture("bronze");
    SetShaderMaterial("bronzeMaterial");
    m_basicMeshes->DrawSphereMesh();
}
*/