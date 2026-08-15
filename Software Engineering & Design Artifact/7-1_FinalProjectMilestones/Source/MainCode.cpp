#include <iostream>
#include <cstdlib>

#include <GL/glew.h>
#include "GLFW/glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneManager.h"
#include "ShaderManager.h"

namespace
{
    const char* const WINDOW_TITLE = "7-1 FinalProject and Milestones";

    GLFWwindow* g_Window = nullptr;
    SceneManager* g_SceneManager = nullptr;
    ShaderManager* g_ShaderManager = nullptr;
}

bool InitializeGLFW();
bool InitializeGLEW();

int main(int argc, char* argv[])
{
    // Initialize GLFW
    if (!InitializeGLFW())
        return EXIT_FAILURE;

    // Shader manager
    g_ShaderManager = new ShaderManager();

    // SceneManager (creates ViewManager internally)
    g_SceneManager = new SceneManager(g_ShaderManager);

    // Create window through ViewManager
    g_Window = g_SceneManager->GetViewManager()->CreateDisplayWindow(WINDOW_TITLE);
    glfwMakeContextCurrent(g_Window);

    // Initialize GLEW
    if (!InitializeGLEW())
        return EXIT_FAILURE;

    // Load shaders
    g_ShaderManager->LoadShaders(
        "shaders/vertexShader.glsl",
        "shaders/fragmentShader.glsl");
    g_ShaderManager->use();

    // Window size → SceneManager
    int width, height;
    glfwGetFramebufferSize(g_Window, &width, &height);
    g_SceneManager->SetWindowSize(width, height);
    glViewport(0, 0, width, height);

    // Prepare scene (meshes, textures, materials)
    g_SceneManager->PrepareScene();

    // Mouse movement callback
    glfwSetCursorPosCallback(g_Window, [](GLFWwindow* window, double xpos, double ypos) {
        g_SceneManager->ProcessMouseMovement(xpos, ypos);
        });

    // Scroll callback
    glfwSetScrollCallback(g_Window, [](GLFWwindow* win, double xoffset, double yoffset) {
        g_SceneManager->ProcessMouseScroll(yoffset);
        });

    // FPS-style mouse look
    glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Main loop
    while (!glfwWindowShouldClose(g_Window))
    {
        // Keyboard input
        g_SceneManager->ProcessInput(g_Window);

        // Enable depth
        glEnable(GL_DEPTH_TEST);

        // Clear screen
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Upload view + projection
        g_SceneManager->PrepareSceneView();

        // Draw scene
        g_SceneManager->RenderScene();

        // Swap buffers
        glfwSwapBuffers(g_Window);
        glfwPollEvents();
    }

    // Cleanup — SceneManager deletes ViewManager internally
    delete g_SceneManager;
    delete g_ShaderManager;

    exit(EXIT_SUCCESS);
}

bool InitializeGLFW()
{
    glfwInit();

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    return true;
}

bool InitializeGLEW()
{
    GLenum result = glewInit();
    if (result != GLEW_OK)
    {
        std::cerr << glewGetErrorString(result) << std::endl;
        return false;
    }

    std::cout << "INFO: OpenGL Successfully Initialized\n";
    std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << "\n\n";

    return true;
}
